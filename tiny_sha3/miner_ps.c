#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

// Control register addresses (adjust for your memory map)
#define MINER_BASE_ADDR 0x43C00000
#define REG_START       (MINER_BASE_ADDR + 0x00)
#define REG_STOP        (MINER_BASE_ADDR + 0x04)
#define REG_STATUS      (MINER_BASE_ADDR + 0x08)
#define REG_NONCE       (MINER_BASE_ADDR + 0x0C)
#define REG_TARGET      (MINER_BASE_ADDR + 0x10)
#define REG_RESULT      (MINER_BASE_ADDR + 0x14)
#define REG_HASH_COUNT_LOW  (MINER_BASE_ADDR + 0x18)
#define REG_HASH_COUNT_HIGH (MINER_BASE_ADDR + 0x1C)

// Mining timeout in seconds
#define MINING_TIMEOUT_SECONDS 15

void start_mining(uint32_t initial_nonce, uint32_t target) {
    printf("Starting mining with nonce=%u, target=%u\n", initial_nonce, target);
    
    // Clear any previous state
    *(volatile uint32_t*)REG_STOP = 1;
    usleep(1000);  // 1ms delay to ensure stop is processed
    *(volatile uint32_t*)REG_STOP = 0;
    
    // Set parameters and start
    *(volatile uint32_t*)REG_NONCE = initial_nonce;
    *(volatile uint32_t*)REG_TARGET = target;
    *(volatile uint32_t*)REG_START = 1;
}

void stop_mining() {
    printf("Stopping mining...\n");
    *(volatile uint32_t*)REG_STOP = 1;
    
    // Wait for miner to acknowledge stop
    int timeout_counter = 1000;  // Prevent infinite wait
    while (timeout_counter-- > 0) {
        uint32_t status = *(volatile uint32_t*)REG_STATUS;
        if (status == 0 || status == 3) {  // Idle or stopped
            break;
        }
        usleep(1000);  // 1ms
    }
    
    // Clear stop flag
    *(volatile uint32_t*)REG_STOP = 0;
}

uint32_t check_mining_result() {
    uint32_t status = *(volatile uint32_t*)REG_STATUS;
    
    if (status == 2) {
        uint32_t result = *(volatile uint32_t*)REG_RESULT;
        printf("Solution found! Nonce: %u\n", result);
        return result;
    }
    return 0;
}

uint64_t get_hash_count() {
    uint32_t low = *(volatile uint32_t*)REG_HASH_COUNT_LOW;
    uint32_t high = *(volatile uint32_t*)REG_HASH_COUNT_HIGH;
    return ((uint64_t)high << 32) | low;
}

void print_mining_status() {
    uint32_t status = *(volatile uint32_t*)REG_STATUS;
    uint64_t hash_count = get_hash_count();
    
    const char* status_str;
    switch(status) {
        case 0: status_str = "IDLE"; break;
        case 1: status_str = "RUNNING"; break;
        case 2: status_str = "FOUND"; break;
        case 3: status_str = "STOPPED"; break;
        default: status_str = "UNKNOWN"; break;
    }
    
    printf("Status: %s, Hashes: %llu\n", status_str, (unsigned long long)hash_count);
}

int main() {
    struct timespec start_time, current_time;
    uint32_t initial_nonce = 1;
    uint32_t target = 1000000005;  // 1e9 + 5
    
    printf("=== SHA-3 Cryptocurrency Miner ===\n");
    printf("Timeout: %d seconds\n", MINING_TIMEOUT_SECONDS);
    
    // Record start time
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0) {
        perror("Failed to get start time");
        return 1;
    }
    
    // Start mining
    start_mining(initial_nonce, target);
    
    // Mining loop with timeout
    uint32_t result = 0;
    int status_update_counter = 0;
    
    while (1) {
        // Check for solution
        result = check_mining_result();
        if (result != 0) {
            printf("Mining completed successfully!\n");
            break;
        }
        
        // Check timeout
        if (clock_gettime(CLOCK_MONOTONIC, &current_time) != 0) {
            perror("Failed to get current time");
            break;
        }
        
        double elapsed_time = (current_time.tv_sec - start_time.tv_sec) + 
                             (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
        
        if (elapsed_time >= MINING_TIMEOUT_SECONDS) {
            printf("Mining timeout reached after %.1f seconds\n", elapsed_time);
            stop_mining();
            
            // Print final statistics
            uint64_t final_hash_count = get_hash_count();
            double hash_rate = final_hash_count / elapsed_time;
            printf("Final hash count: %llu\n", (unsigned long long)final_hash_count);
            printf("Average hash rate: %.0f H/s\n", hash_rate);
            break;
        }
        
        // Print status every 100 iterations (~1 second)
        if (status_update_counter % 100 == 0) {
            printf("[%.1fs] ", elapsed_time);
            print_mining_status();
        }
        status_update_counter++;
        
        // Small delay to prevent excessive CPU usage
        usleep(10000);  // 10ms
    }
    
    // Final result
    if (result != 0) {
        printf("Result: %u\n", result);
        
        // Calculate final statistics
        if (clock_gettime(CLOCK_MONOTONIC, &current_time) == 0) {
            double total_time = (current_time.tv_sec - start_time.tv_sec) + 
                               (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
            uint64_t final_hash_count = get_hash_count();
            double hash_rate = final_hash_count / total_time;
            
            printf("Mining time: %.2f seconds\n", total_time);
            printf("Total hashes: %llu\n", (unsigned long long)final_hash_count);
            printf("Hash rate: %.0f H/s\n", hash_rate);
        }
        
        return 0;  // Success
    } else {
        printf("Result: No solution found within timeout\n");
        return 1;  // Timeout/failure
    }
}

