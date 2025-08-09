#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>

// Physical base address and size
#define MINER_PHYS_ADDR 0xA0010000U
#define MINER_SIZE      0x1000  // 4KB should be enough

// Register offsets
#define REG_START_OFFSET       0x28  // control_start_i
#define REG_STOP_OFFSET        0x38  // control_stop_i  
#define REG_STATUS_OFFSET      0x48  // control_status
#define REG_NONCE_OFFSET       0x58  // control_initial_nonce
#define REG_TARGET_OFFSET      0x10  // control_target_hash
#define REG_RESULT_OFFSET      0x60  // control_result_nonce
#define REG_HASH_COUNT_LOW_OFFSET  0x18  // control_hash_count_low
#define REG_HASH_COUNT_HIGH_OFFSET 0x70  // control_hash_count_high

// +++ ADD +++
#define REG_CTRL_OFFSET   0x00
#define CTRL_AP_START     (1u<<0)
#define CTRL_AP_DONE      (1u<<1)
#define CTRL_AP_IDLE      (1u<<2)
#define CTRL_AP_READY     (1u<<3)
#define CTRL_AUTO_RESTART (1u<<7)
// --- ADD ---

// Mining timeout in seconds
#define MINING_TIMEOUT_SECONDS 15

// Global pointer to mapped memory
static volatile uint32_t *miner_regs = NULL;

int init_miner_interface() {
    int fd;
    
    // Try UIO first (preferred method)
    fd = open("/dev/uio0", O_RDWR);
    if (fd >= 0) {
        printf("Using UIO driver\n");
        miner_regs = mmap(NULL, MINER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (miner_regs == MAP_FAILED) {
            printf("Failed to mmap UIO device\n");
            close(fd);
            return -1;
        }
        return fd;
    }
    
    // Fallback to /dev/mem (requires root privileges)
    printf("UIO not available, trying /dev/mem (requires root)\n");
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Failed to open /dev/mem - are you running as root?");
        return -1;
    }
    
    miner_regs = mmap(NULL, MINER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MINER_PHYS_ADDR);
    if (miner_regs == MAP_FAILED) {
        perror("Failed to mmap /dev/mem");
        close(fd);
        return -1;
    }
    
    printf("Successfully mapped miner registers at %p\n", (void*)miner_regs);
    return fd;
}

void cleanup_miner_interface(int fd) {
    if (miner_regs != NULL) {
        munmap((void*)miner_regs, MINER_SIZE);
        miner_regs = NULL;
    }
    if (fd >= 0) {
        close(fd);
    }
}

// Helper macros for register access
#define READ_REG(offset) (miner_regs[(offset)/4])
#define WRITE_REG(offset, value) (miner_regs[(offset)/4] = (value))

void start_mining(uint32_t initial_nonce, uint32_t target) {
    if (!miner_regs) {
        printf("Error: Miner interface not initialized\n");
        return;
    }
    
    printf("Starting mining with nonce=%u, target=%u\n", initial_nonce, target);
    
    // Clear any previous state
    WRITE_REG(REG_STOP_OFFSET, 1);
    usleep(1000);  // 1ms delay to ensure stop is processed
    WRITE_REG(REG_STOP_OFFSET, 0);

    // +++ ADD +++ //
    WRITE_REG(REG_NONCE_OFFSET,  initial_nonce);
    WRITE_REG(REG_TARGET_OFFSET, target);

    WRITE_REG(0x04, 0x0);
    WRITE_REG(0x0C, 0x1);

    WRITE_REG(REG_CTRL_OFFSET, CTRL_AP_START | CTRL_AUTO_RESTART);

    WRITE_REG(REG_START_OFFSET, 1);
    // --- ADD --- //
    
    // // Set parameters and start
    // WRITE_REG(REG_NONCE_OFFSET, initial_nonce);
    // WRITE_REG(REG_TARGET_OFFSET, target);
    // WRITE_REG(REG_START_OFFSET, 1);
}

// void stop_mining() {
//     if (!miner_regs) return;
    
//     printf("Stopping mining...\n");
//     WRITE_REG(REG_STOP_OFFSET, 1);
    
//     // Wait for miner to acknowledge stop
//     int timeout_counter = 1000;  // Prevent infinite wait
//     while (timeout_counter-- > 0) {
//         uint32_t status = READ_REG(REG_STATUS_OFFSET);
//         if (status == 0 || status == 3) {  // Idle or stopped
//             break;
//         }
//         usleep(1000);  // 1ms
//     }
    
//     // Clear stop flag
//     WRITE_REG(REG_STOP_OFFSET, 0);
// }

// +++ ADD +++ //
void stop_mining(void) {
    if (!miner_regs) return;

    uint32_t ctrl = READ_REG(REG_CTRL_OFFSET);
    if (ctrl & CTRL_AUTO_RESTART) {
        WRITE_REG(REG_CTRL_OFFSET, ctrl & ~CTRL_AUTO_RESTART);
    }

    WRITE_REG(REG_STOP_OFFSET, 1);

    for (int i = 0; i < 1000; ++i) {
        uint32_t s = READ_REG(REG_STATUS_OFFSET);
        if (s == 0 || s == 3) break;
        usleep(1000);
    }

    WRITE_REG(REG_STOP_OFFSET, 0);
}
// --- ADD ---//

uint32_t check_mining_result() {
    if (!miner_regs) return 0;
    
    uint32_t status = READ_REG(REG_STATUS_OFFSET);
    
    if (status == 2) {
        uint32_t result = READ_REG(REG_RESULT_OFFSET);
        printf("Solution found! Nonce: %u\n", result);
        return result;
    }
    return 0;
}

uint64_t get_hash_count() {
    if (!miner_regs) return 0;
    
    uint32_t low = READ_REG(REG_HASH_COUNT_LOW_OFFSET);
    uint32_t high = READ_REG(REG_HASH_COUNT_HIGH_OFFSET);
    return ((uint64_t)high << 32) | low;
}

void print_mining_status() {
    if (!miner_regs) return;
    
    uint32_t status = READ_REG(REG_STATUS_OFFSET);
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
    int fd;
    
    printf("=== SHA-3 Cryptocurrency Miner ===\n");
    printf("Timeout: %d seconds\n", MINING_TIMEOUT_SECONDS);
    
    // Try UIO first, then /dev/mem
    fd = open("/dev/uio0", O_RDWR);
    if (fd >= 0) {
        printf("Using UIO driver\n");
        miner_regs = mmap(NULL, MINER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (miner_regs == MAP_FAILED) {
            printf("UIO mmap failed, trying /dev/mem\n");
            close(fd);
            fd = -1;
        }
    }
    
    if (fd < 0) {
        printf("Trying /dev/mem (requires root)\n");
        fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd < 0) {
            perror("Failed to open /dev/mem");
            return 1;
        }
        
        miner_regs = mmap(NULL, MINER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MINER_PHYS_ADDR);
        if (miner_regs == MAP_FAILED) {
            perror("Failed to mmap /dev/mem");
            close(fd);
            return 1;
        }
    }
    
    printf("Successfully mapped miner registers\n");
    
    // Record start time
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0) {
        perror("Failed to get start time");
        cleanup_miner_interface(fd);
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
        
        cleanup_miner_interface(fd);
        return 0;  // Success
    } else {
        printf("Result: No solution found within timeout\n");
        cleanup_miner_interface(fd);
        return 1;  // Timeout/failure
    }
}
