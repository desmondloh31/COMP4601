// HLS-Friendly SHA-3 Implementation
// Addresses HLS synthesis issues with unions, arbitrary arrays, and dynamic indexing

#ifndef SHA3_HLS_H
#define SHA3_HLS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// HLS-specific includes
#ifdef __SYNTHESIS__
#include "ap_int.h"
#include "hls_stream.h"
#endif

#ifndef KECCAKF_ROUNDS
#define KECCAKF_ROUNDS 24
#define SHA3_256_OUTPUT_BYTES 32
#define SHA3_256_RATE 136  // 200 - 2*32 = 136 bytes
#define MAX_PIPELINE_DEPTH 8
#endif

#ifndef ROTL64
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

// Miner control registers (AXI-Lite interface)
typedef struct {
    uint32_t start;           // Start mining (write 1 to start)
    uint32_t stop;            // Stop mining (write 1 to stop)
    uint32_t status;          // Status: 0=idle, 1=running, 2=found, 3=error
    uint32_t initial_nonce;   // Starting nonce value
    uint32_t target_hash;     // Target hash value (simplified to uint32 for demo)
    uint32_t result_nonce;    // Found nonce (valid when status=2)
    uint32_t hash_count_low;  // Hash counter (low 32 bits)
    uint32_t hash_count_high; // Hash counter (high 32 bits)
} miner_control_t;

// HLS-friendly context structure - NO UNION
typedef struct {
    uint32_t nonce;         // 64-bit state words (replaces union)
    uint32_t hash_value;       // Separate byte buffer for input processing
    int      valid;             // Indicates if buffer contains valid data
} hash_result_t;

// Fixed-size message processing (HLS-friendly)
void sha3_keccakf_hls(uint64_t state[25]);

void hash_nonce(uint32_t nonce, uint32_t *hash_output, bool *valid);

bool compare_hash(uint32_t hash_value, uint32_t target);

uint32_t increment_nonce(uint32_t current_nonce);

void mining_pipeline(
    uint32_t initial_nonce,
    uint32_t target,
    bool start_mining,
    bool *found_solution,
    uint32_t *solution_nonce,
    uint64_t *hash_count
);

void sha3_miner_top(
    // AXI-Lite control interface
    uint32_t *control_start,
    uint32_t *control_stop, 
    uint32_t *control_status,
    uint32_t *control_initial_nonce,
    uint32_t *control_target_hash,
    uint32_t *control_result_nonce,
    uint32_t *control_hash_count_low,
    uint32_t *control_hash_count_high
);

void sha3_miner_batch(
    uint32_t initial_nonce,
    uint32_t target_hash,
    uint32_t max_iterations,
    uint32_t *result_nonce,
    bool *found_solution,
    uint32_t *iterations_completed
);

#endif

