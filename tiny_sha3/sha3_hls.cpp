#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include <ap_int.h>
#include <hls_stream.h>
#include "sha3_hls.h"


void sha3_keccakf_hls(uint64_t state[25])
{
    #pragma HLS INLINE off
    #pragma HLS ARRAY_PARTITION variable=state complete dim=1
    
    // Keccak constants
    static const uint64_t keccakf_rndc[24] = {
        0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
        0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
        0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
        0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
        0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
        0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
        0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
        0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
    };
    #pragma HLS RESOURCE variable=keccakf_rndc core=ROM_1P_BRAM

    static const int keccakf_rotc[24] = {
        1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
        27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
    };
    #pragma HLS RESOURCE variable=keccakf_rotc core=ROM_1P_BRAM

    static const int keccakf_piln[24] = {
        10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
        15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
    };
    #pragma HLS RESOURCE variable=keccakf_piln core=ROM_1P_BRAM

    uint64_t t, bc[5];
    #pragma HLS ARRAY_PARTITION variable=bc complete dim=1

    // Keccak-f rounds
    ROUND_LOOP: for (int r = 0; r < KECCAKF_ROUNDS; r++) {
        #pragma HLS PIPELINE off
        
        // Theta columns
        THETA_1: for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            bc[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];
        }
        
        // Theta rows
        THETA_2: for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
            state[i]      ^= t;
            state[i + 5]  ^= t; 
            state[i + 10] ^= t;
            state[i + 15] ^= t;
            state[i + 20] ^= t;
        }

        t = state[1];
        RHO_PI: for (int i = 0; i < 24; i++) {
            #pragma HLS PIPELINE II=1
            int j = keccakf_piln[i];
            bc[0] = state[j];
            state[j] = ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        // Chi (unrolled for all 5 rows)
        CHI_ROW: for (int row = 0; row < 5; row++) {
            #pragma HLS UNROLL
            int base = row * 5;
            for (int i = 0; i < 5; i++) {
                #pragma HLS UNROLL
                bc[i] = state[base + i];
            }
            for (int i = 0; i < 5; i++) {
                #pragma HLS UNROLL
                state[base + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
            }
        }

        // Iota
        state[0] ^= keccakf_rndc[r];
    }
}

// Hash a nonce (adder-hasher component)
void hash_nonce(uint32_t nonce, uint32_t *hash_output, bool *valid)
{
    #pragma HLS INLINE off
    #pragma HLS PIPELINE off
    
    // Initialize SHA-3 state
    uint64_t state[25];
    #pragma HLS ARRAY_PARTITION variable=state complete dim=1
    
    INIT_STATE: for (int i = 0; i < 25; i++) {
        #pragma HLS UNROLL
        state[i] = 0;
    }
    
    // Absorb nonce (simplified - just place nonce in first word)
    // Real mining include block header + nonce
    // For sake of demonstration we ignore it here
    state[0] ^= (uint64_t)nonce;
    
    // Apply padding for SHA-3 
    state[0] ^= 0x06ULL << 32;  // Domain separation  
    state[(SHA3_256_RATE/8) - 1] ^= 0x8000000000000000ULL;
    
    sha3_keccakf_hls(state);
    
    // Extract first 32 bits of hash for comparison
    *hash_output = (uint32_t)(state[0] & 0xFFFFFFFFULL);
    *valid = true;
}

// Compare hash with target (comparator component)
bool compare_hash(uint32_t hash_value, uint32_t target)
{
    #pragma HLS INLINE
    return hash_value < target;
}

// Nonce incrementer
uint32_t increment_nonce(uint32_t current_nonce)
{
    #pragma HLS INLINE
    return current_nonce + 1;
}

// Desynchronized mining pipeline
void mining_pipeline(
    uint32_t initial_nonce,
    uint32_t target,
    bool start_mining,
    bool *found_solution,
    uint32_t *solution_nonce,
    uint64_t *hash_count
)
{
    // Pipeline state
    static uint32_t current_nonce = 0;
    static uint64_t total_hashes = 0;
    static bool pipeline_active = false;
    
    // Pipeline registers for desynchronization
    // BACKUP PIPELINE BUFFER if somehow comparator lags adder-hasher
    static hash_result_t pipeline_stage[MAX_PIPELINE_DEPTH];
    #pragma HLS ARRAY_PARTITION variable=pipeline_stage complete dim=1
    static int pipeline_head = 0;
    static int pipeline_tail = 0;
    
    *found_solution = false;
    *solution_nonce = 0;
    *hash_count = total_hashes;
    
    // Initialize on start
    if (start_mining && !pipeline_active) {
        current_nonce = initial_nonce;
        total_hashes = 0;
        pipeline_active = true;
        pipeline_head = 0;
        pipeline_tail = 0;
        
        // Clear pipeline
        CLEAR_PIPELINE: for (int i = 0; i < MAX_PIPELINE_DEPTH; i++) {
            #pragma HLS UNROLL
            pipeline_stage[i].valid = false;
        }
    }
    
    if (!pipeline_active) {
        return;
    }
    
    // ======== ADDER - HASHER ========
    // Hash current nonce 
    uint32_t hash_result;
    bool hash_valid;
    hash_nonce(current_nonce, &hash_result, &hash_valid);
    
    if (hash_valid) {
        // Insert into pipeline
        pipeline_stage[pipeline_head].nonce = current_nonce;
        pipeline_stage[pipeline_head].hash_value = hash_result;
        pipeline_stage[pipeline_head].valid = true;
        pipeline_head = (pipeline_head + 1) % MAX_PIPELINE_DEPTH;
        
        // Increment nonce for next iteration
        current_nonce = increment_nonce(current_nonce);
        total_hashes++;
    }

    // ======== COMPARATOR ========
    
    // Check pipeline tail 
    if (pipeline_stage[pipeline_tail].valid) {
        hash_result_t current_result = pipeline_stage[pipeline_tail];
        pipeline_stage[pipeline_tail].valid = false;
        pipeline_tail = (pipeline_tail + 1) % MAX_PIPELINE_DEPTH;
        
        // Compare with target
        if (compare_hash(current_result.hash_value, target)) {
            *found_solution = true;
            *solution_nonce = current_result.nonce;
            pipeline_active = false;
        }
    }
    
    *hash_count = total_hashes;
}

// Top-level mining controller (AXI-Lite interface)
void sha3_miner_top(
    uint32_t *control_start,
    uint32_t *control_stop, 
    uint32_t *control_status,
    uint32_t *control_initial_nonce,
    uint32_t *control_target_hash,
    uint32_t *control_result_nonce,
    uint32_t *control_hash_count_low,
    uint32_t *control_hash_count_high
)
{

    #pragma HLS INTERFACE mode=s_axilite port=control_start bundle=control offset=0x00
    #pragma HLS INTERFACE mode=s_axilite port=control_stop bundle=control offset=0x04
    #pragma HLS INTERFACE mode=s_axilite port=control_status bundle=control offset=0x08
    #pragma HLS INTERFACE mode=s_axilite port=control_initial_nonce bundle=control offset=0x0C
    #pragma HLS INTERFACE mode=s_axilite port=control_target_hash bundle=control offset=0x10
    #pragma HLS INTERFACE mode=s_axilite port=control_result_nonce bundle=control offset=0x14
    #pragma HLS INTERFACE mode=s_axilite port=control_hash_count_low bundle=control offset=0x18
    #pragma HLS INTERFACE mode=s_axilite port=control_hash_count_high bundle=control offset=0x1C
    #pragma HLS INTERFACE mode=s_axilite port=return bundle=control

    
    // Static state for persistent operation
    static uint32_t miner_status = 0;  // 0=idle, 1=running, 2=found, 3=stopped
    static bool mining_active = false;
    static uint32_t found_nonce = 0;
    static uint64_t hash_counter = 0;
    
    // Read control inputs
    bool start_requested = (*control_start == 1);
    bool stop_requested = (*control_stop == 1);
    uint32_t initial_nonce = *control_initial_nonce;
    uint32_t target_hash = *control_target_hash;
    
    // State machine
    if (start_requested && miner_status == 0) {
        miner_status = 1;
        mining_active = true;
        hash_counter = 0;
        found_nonce = 0;
        *control_start = 0;  // Clear start flag
    } else if (stop_requested) {
        miner_status = 3;
        mining_active = false;
        *control_stop = 0;  // Clear stop flag
    }
    
    // Run mining pipeline
    if (mining_active) {
        bool solution_found;
        uint32_t solution_nonce;
        uint64_t current_hash_count;
        
        mining_pipeline(
            initial_nonce,
            target_hash,
            true,
            &solution_found,
            &solution_nonce,
            &current_hash_count
        );
        
        hash_counter = current_hash_count;
        
        if (solution_found) {
            miner_status = 2;
            mining_active = false;
            found_nonce = solution_nonce;
        }
    }
    
    // Update control outputs
    *control_status = miner_status;
    *control_result_nonce = found_nonce;
    *control_hash_count_low = (uint32_t)(hash_counter & 0xFFFFFFFFULL);
    *control_hash_count_high = (uint32_t)((hash_counter >> 32) & 0xFFFFFFFFULL);
    
    // Auto-reset to idle when solution found or stopped
    if (miner_status == 2 || miner_status == 3) {
        // Wait for acknowledgment (start flag cleared) before going idle
        if (*control_start == 0 && *control_stop == 0) {
            miner_status = 0;
        }
    }
}


// High-performance batch mining 
// Originally designed for axi-stream, but we couldn't get it working :(
/*
void sha3_miner_batch(
    uint32_t initial_nonce,
    uint32_t target_hash,
    uint32_t max_iterations,
    uint32_t *result_nonce,
    bool *found_solution,
    uint32_t *iterations_completed
)
{
    #pragma HLS INTERFACE mode=s_axilite port=initial_nonce
    #pragma HLS INTERFACE mode=s_axilite port=target_hash
    #pragma HLS INTERFACE mode=s_axilite port=max_iterations
    #pragma HLS INTERFACE mode=s_axilite port=result_nonce
    #pragma HLS INTERFACE mode=s_axilite port=found_solution
    #pragma HLS INTERFACE mode=s_axilite port=iterations_completed
    #pragma HLS INTERFACE mode=s_axilite port=return
    
    *found_solution = false;
    *result_nonce = 0;
    
    MINING_LOOP: for (uint32_t i = 0; i < max_iterations; i++) {
        #pragma HLS PIPELINE II=25  // Adjust based on hash function latency
        #pragma HLS LOOP_TRIPCOUNT min=1000 max=1000000
        
        uint32_t current_nonce = initial_nonce + i;
        uint32_t hash_value;
        bool hash_valid;
        
        hash_nonce(current_nonce, &hash_value, &hash_valid);
        
        // Check if solution found
        if (hash_valid && compare_hash(hash_value, target_hash)) {
            *found_solution = true;
            *result_nonce = current_nonce;
            *iterations_completed = i + 1;
            return;
        }
    }
    
    *iterations_completed = max_iterations;
}
*/
