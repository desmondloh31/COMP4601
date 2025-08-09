#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Mock HLS headers for simulation
#ifndef __SYNTHESIS__
//#define ap_int int
//typedef struct {} hls_stream;
#endif

// Include your header
#include "sha3_hls.h"

// Test utilities
void print_state(const char* test_name, uint32_t status, uint32_t result_nonce, uint64_t hash_count) {
    const char* status_str[] = {"IDLE", "RUNNING", "FOUND", "STOPPED"};
    printf("[%s] Status: %s, Result Nonce: 0x%08X, Hash Count: %llu\n", 
           test_name, 
           (status < 4) ? status_str[status] : "UNKNOWN",
           result_nonce, 
           (unsigned long long)hash_count);
}

void print_hex_array(const char* label, uint8_t* data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

// Test 1: Basic SHA3 Keccak-f function test
void test_keccakf() {
    printf("\n=== Test 1: SHA3 Keccak-f Function ===\n");
    
    uint64_t state[25];
    // Initialize with a simple pattern
    for (int i = 0; i < 25; i++) {
        state[i] = i;
    }
    
    printf("Initial state[0]: 0x%016llx\n", (unsigned long long)state[0]);
    printf("Initial state[1]: 0x%016llx\n", (unsigned long long)state[1]);
    
    sha3_keccakf_hls(state);
    
    printf("After Keccak-f state[0]: 0x%016llx\n", (unsigned long long)state[0]);
    printf("After Keccak-f state[1]: 0x%016llx\n", (unsigned long long)state[1]);
    
    // Basic sanity check - state should have changed
    bool changed = false;
    for (int i = 0; i < 25; i++) {
        if (state[i] != i) {
            changed = true;
            break;
        }
    }
    printf("Keccak-f function: %s\n", changed ? "PASS - State modified" : "FAIL - State unchanged");
}

// Test 2: Single nonce hashing
void test_single_hash() {
    printf("\n=== Test 2: Single Nonce Hashing ===\n");
    
    uint32_t test_nonces[] = {0, 1, 0x12345678, 0xFFFFFFFF};
    int num_tests = sizeof(test_nonces) / sizeof(test_nonces[0]);
    
    for (int i = 0; i < num_tests; i++) {
        uint32_t hash_output;
        bool valid;
        
        hash_nonce(test_nonces[i], &hash_output, &valid);
        
        printf("Nonce: 0x%08X -> Hash: 0x%08X, Valid: %s\n", 
               test_nonces[i], hash_output, valid ? "true" : "false");
    }
    
    // Test that different nonces produce different hashes
    uint32_t hash1, hash2;
    bool valid1, valid2;
    
    hash_nonce(0, &hash1, &valid1);
    hash_nonce(1, &hash2, &valid2);
    
    printf("Hash determinism test: %s\n", 
           (hash1 != hash2) ? "PASS - Different nonces produce different hashes" : 
                             "FAIL - Same hash for different nonces");
}

// Test 3: Hash comparison
void test_hash_comparison() {
    printf("\n=== Test 3: Hash Comparison ===\n");
    
    uint32_t hash = 0x12345678;
    uint32_t targets[] = {0x00000000, 0x12345677, 0x12345678, 0x12345679, 0xFFFFFFFF};
    bool expected[] = {false, false, false, true, true};
    
    for (int i = 0; i < 5; i++) {
        bool result = compare_hash(hash, targets[i]);
        printf("Hash 0x%08X < Target 0x%08X: %s (%s)\n", 
               hash, targets[i], 
               result ? "true" : "false",
               (result == expected[i]) ? "PASS" : "FAIL");
    }
}

// Test 4: Mining pipeline with easy target
void test_mining_pipeline_easy() {
    printf("\n=== Test 4: Mining Pipeline - Easy Target ===\n");
    
    uint32_t initial_nonce = 0;
    uint32_t target = 0x80000000; // Should be relatively easy to find
    bool found_solution = false;
    uint32_t solution_nonce = 0;
    uint64_t hash_count = 0;
    int max_iterations = 1000;
    
    printf("Starting mining with easy target 0x%08X\n", target);
    
    for (int iter = 0; iter < max_iterations && !found_solution; iter++) {
        mining_pipeline(
            initial_nonce,
            target,
            true,
            &found_solution,
            &solution_nonce,
            &hash_count
        );
        
        if (iter % 100 == 0) {
            printf("Iteration %d: Hash count: %llu\n", iter, (unsigned long long)hash_count);
        }
    }
    
    if (found_solution) {
        printf("PASS - Solution found! Nonce: 0x%08X, Hash count: %llu\n", 
               solution_nonce, (unsigned long long)hash_count);
        
        // Verify the solution
        uint32_t verify_hash;
        bool valid;
        hash_nonce(solution_nonce, &verify_hash, &valid);
        bool is_valid = valid && compare_hash(verify_hash, target);
        printf("Solution verification: %s (Hash: 0x%08X)\n", 
               is_valid ? "PASS" : "FAIL", verify_hash);
    } else {
        printf("FAIL - No solution found in %d iterations\n", max_iterations);
    }
}

// Test 5: AXI-Lite interface simulation
void test_axi_interface() {
    printf("\n=== Test 5: AXI-Lite Interface Simulation ===\n");
    
    // AXI-Lite registers
    uint32_t control_start = 0;
    uint32_t control_stop = 0;
    uint32_t control_status = 0;
    uint32_t control_initial_nonce = 0;
    uint32_t control_target_hash = 0x80000000; // Easy target
    uint32_t control_result_nonce = 0;
    uint32_t control_hash_count_low = 0;
    uint32_t control_hash_count_high = 0;
    
    printf("Step 1: Check initial idle state\n");
    sha3_miner_top(&control_start, &control_stop, &control_status,
                   &control_initial_nonce, &control_target_hash,
                   &control_result_nonce, &control_hash_count_low, &control_hash_count_high);
    
    uint64_t hash_count = ((uint64_t)control_hash_count_high << 32) | control_hash_count_low;
    print_state("Initial", control_status, control_result_nonce, hash_count);
    
    printf("\nStep 2: Start mining\n");
    control_start = 1;
    control_initial_nonce = 0;
    
    // Run for multiple cycles to simulate mining
    int max_cycles = 2000;
    for (int cycle = 0; cycle < max_cycles; cycle++) {
        sha3_miner_top(&control_start, &control_stop, &control_status,
                       &control_initial_nonce, &control_target_hash,
                       &control_result_nonce, &control_hash_count_low, &control_hash_count_high);
        
        hash_count = ((uint64_t)control_hash_count_high << 32) | control_hash_count_low;
        
        if (cycle % 200 == 0) {
            print_state("Mining", control_status, control_result_nonce, hash_count);
        }
        
        // Check if solution found
        if (control_status == 2) { // FOUND
            printf("\nSolution found at cycle %d!\n", cycle);
            print_state("Solution", control_status, control_result_nonce, hash_count);
            
            // Verify the solution
            uint32_t verify_hash;
            bool valid;
            hash_nonce(control_result_nonce, &verify_hash, &valid);
            bool is_valid = valid && compare_hash(verify_hash, control_target_hash);
            printf("Verification: %s (Hash: 0x%08X vs Target: 0x%08X)\n", 
                   is_valid ? "PASS" : "FAIL", verify_hash, control_target_hash);
            break;
        }
        
        // Check if stopped unexpectedly
        if (control_status == 3) { // STOPPED
            printf("\nMining stopped unexpectedly at cycle %d\n", cycle);
            break;
        }
    }
    
    if (control_status == 1) { // Still running
        printf("\nStep 3: Stop mining (timeout after %d cycles)\n", max_cycles);
        control_stop = 1;
        sha3_miner_top(&control_start, &control_stop, &control_status,
                       &control_initial_nonce, &control_target_hash,
                       &control_result_nonce, &control_hash_count_low, &control_hash_count_high);
        
        hash_count = ((uint64_t)control_hash_count_high << 32) | control_hash_count_low;
        print_state("Stopped", control_status, control_result_nonce, hash_count);
    }
}

// Test 6: Stress test with very easy target
void test_stress_easy_target() {
    printf("\n=== Test 6: Stress Test - Very Easy Target ===\n");
    
    uint32_t control_start = 1;
    uint32_t control_stop = 0;
    uint32_t control_status = 0;
    uint32_t control_initial_nonce = 0;
    uint32_t control_target_hash = 40000000; // Very easy target
    uint32_t control_result_nonce = 0;
    uint32_t control_hash_count_low = 0;
    uint32_t control_hash_count_high = 0;
    
    printf("Using very easy target: 0x%08X\n", control_target_hash);
    
    int found_solutions = 0;
    int max_runs = 5;
    
    for (int run = 0; run < max_runs; run++) {
        printf("\n--- Run %d ---\n", run + 1);
        
        // Reset for new run
        control_start = 1;
        control_stop = 0;
        control_initial_nonce = run * 1000; // Different starting point each run
        
        for (int cycle = 0; cycle < 2000; cycle++) {
            sha3_miner_top(&control_start, &control_stop, &control_status,
                           &control_initial_nonce, &control_target_hash,
                           &control_result_nonce, &control_hash_count_low, &control_hash_count_high);
            
            if (control_status == 2) { // Found solution
                uint64_t hash_count = ((uint64_t)control_hash_count_high << 32) | control_hash_count_low;
                printf("Status:%d, %d\n", control_initial_nonce, control_target_hash);
                printf("Solution %d found: Nonce 0x%08X, Cycles: %d, Hashes: %llu\n", 
                       run + 1, control_result_nonce, cycle, (unsigned long long)hash_count);
                found_solutions++;
                break;
            }
        }
        
        if (control_status != 2) {
            printf("Run %d: No solution found\n", run + 1);
        }
        
        // Reset state for next run
        control_start = 0;
        control_stop = 0;
        // Run a few more cycles to reset internal state
        for (int i = 0; i < 5; i++) {
            sha3_miner_top(&control_start, &control_stop, &control_status,
                           &control_initial_nonce, &control_target_hash,
                           &control_result_nonce, &control_hash_count_low, &control_hash_count_high);
        }
    }
    
    printf("\nStress test results: %d/%d solutions found\n", found_solutions, max_runs);
}

int main() {
    printf("=======================================================\n");
    printf("SHA3 Miner HLS Testbench\n");
    printf("=======================================================\n");
    
    // Run all tests
    test_keccakf();
    test_single_hash();
    test_hash_comparison();
    test_mining_pipeline_easy();
    test_axi_interface();
    test_stress_easy_target();
    
    printf("\n=======================================================\n");
    printf("All tests completed. Check results above.\n");
    printf("=======================================================\n");
    
    return 0;
}

