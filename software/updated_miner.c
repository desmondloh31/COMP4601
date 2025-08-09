#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>

// =======================
// Mining Register Map
// =======================
#define MINER_BASE_ADDR      0xA0010000U
#define MAP_SIZE             4096UL
#define MAP_MASK             (MAP_SIZE - 1)

#define REG_START_OFFSET         0x28
#define REG_STOP_OFFSET          0x38
#define REG_STATUS_OFFSET        0x48
#define REG_NONCE_OFFSET         0x58
#define REG_TARGET_OFFSET        0x10
#define REG_RESULT_OFFSET        0x60
#define REG_HASH_COUNT_LOW_OFFSET  0x18
#define REG_HASH_COUNT_HIGH_OFFSET 0x70

// Mining timeout in seconds
#define MINING_TIMEOUT_SECONDS 15

// Macro to access mapped registers
#define REG(offset) (*(volatile uint32_t *)((char *)map_base + (offset)))

// Status values (example)
#define STATUS_IDLE     0
#define STATUS_RUNNING  1
#define STATUS_FOUND    2
#define STATUS_STOPPED  3

static void *map_base = NULL;

// =======================
// Miner control functions
// =======================
void start_mining(uint32_t initial_nonce, uint32_t target) {
    printf("Starting mining: nonce=%u, target=%u\n", initial_nonce, target);

    REG(REG_STOP_OFFSET) = 1;
    usleep(1000);
    REG(REG_STOP_OFFSET) = 0;

    REG(REG_NONCE_OFFSET) = initial_nonce;
    REG(REG_TARGET_OFFSET) = target;
    REG(REG_START_OFFSET) = 1;
}

void stop_mining() {
    printf("Stopping mining...\n");
    REG(REG_STOP_OFFSET) = 1;

    int timeout_counter = 1000;
    while (timeout_counter-- > 0) {
        uint32_t status = REG(REG_STATUS_OFFSET);
        if (status == STATUS_IDLE || status == STATUS_STOPPED) {
            break;
        }
        usleep(1000);
    }

    REG(REG_STOP_OFFSET) = 0;
}

uint32_t check_mining_result() {
    uint32_t status = REG(REG_STATUS_OFFSET);
    if (status == STATUS_FOUND) {
        uint32_t result = REG(REG_RESULT_OFFSET);
        printf("Solution found! Nonce: %u\n", result);
        return result;
    }
    return 0;
}

uint64_t get_hash_count() {
    uint32_t low = REG(REG_HASH_COUNT_LOW_OFFSET);
    uint32_t high = REG(REG_HASH_COUNT_HIGH_OFFSET);
    return ((uint64_t)high << 32) | low;
}

void print_mining_status() {
    uint32_t status = REG(REG_STATUS_OFFSET);
    uint64_t hash_count = get_hash_count();

    const char* status_str;
    switch (status) {
        case STATUS_IDLE: status_str = "IDLE"; break;
        case STATUS_RUNNING: status_str = "RUNNING"; break;
        case STATUS_FOUND: status_str = "FOUND"; break;
        case STATUS_STOPPED: status_str = "STOPPED"; break;
        default: status_str = "UNKNOWN"; break;
    }

    printf("Status: %s, Hashes: %llu\n", status_str, (unsigned long long)hash_count);
}

// =======================
// Main program
// =======================
int main() {
    int mem_fd;
    struct timespec start_time, current_time;

    // Open /dev/mem
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        perror("open /dev/mem");
        return 1;
    }

    // Map miner registers
    map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, MINER_BASE_ADDR & ~MAP_MASK);
    if (map_base == MAP_FAILED) {
        perror("mmap");
        close(mem_fd);
        return 1;
    }

    uint32_t initial_nonce = 1;
    uint32_t target = 1000000005;

    printf("=== SHA-3 Cryptocurrency Miner ===\n");
    printf("Timeout: %d seconds\n", MINING_TIMEOUT_SECONDS);

    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0) {
        perror("clock_gettime");
        return 1;
    }

    start_mining(initial_nonce, target);

    uint32_t result = 0;
    int status_update_counter = 0;

    while (1) {
        result = check_mining_result();
        if (result != 0) {
            printf("Mining completed successfully!\n");
            break;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &current_time) != 0) {
            perror("clock_gettime");
            break;
        }

        double elapsed_time = (current_time.tv_sec - start_time.tv_sec) +
                              (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

        if (elapsed_time >= MINING_TIMEOUT_SECONDS) {
            printf("Mining timeout reached after %.1f seconds\n", elapsed_time);
            stop_mining();

            uint64_t final_hash_count = get_hash_count();
            double hash_rate = final_hash_count / elapsed_time;
            printf("Final hash count: %llu\n", (unsigned long long)final_hash_count);
            printf("Average hash rate: %.0f H/s\n", hash_rate);
            break;
        }

        if (status_update_counter % 100 == 0) {
            printf("[%.1fs] ", elapsed_time);
            print_mining_status();
        }
        status_update_counter++;
        usleep(10000);
    }

    if (result != 0) {
        printf("Result: %u\n", result);
        if (clock_gettime(CLOCK_MONOTONIC, &current_time) == 0) {
            double total_time = (current_time.tv_sec - start_time.tv_sec) +
                                (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
            uint64_t final_hash_count = get_hash_count();
            double hash_rate = final_hash_count / total_time;

            printf("Mining time: %.2f seconds\n", total_time);
            printf("Total hashes: %llu\n", (unsigned long long)final_hash_count);
            printf("Hash rate: %.0f H/s\n", hash_rate);
        }
    } else {
        printf("Result: No solution found within timeout\n");
    }

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return 0;
}
