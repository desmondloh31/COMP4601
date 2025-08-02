#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>


#define MAP_SIZE       4096UL
#define MAP_MASK       (MAP_SIZE - 1)
#define BASE_ADDR      0xA0000000  // Need to replace with vivado block design base addr or AXI

#define COUNT_MASK        0x7FFFFFFF

#define MAX_COMPARE_COUNT 1000000  

// Register offsets
// Sending 
#define REG_CTRL_READY    0x00 // Sent by PS to start mining/reset comparator
#define REG_NONCE_START   0x04 // Only nonce is sent from PS to header
#define REG_TARGET_LO     0x08 // Sets target_reg
#define REG_TARGET_HI     0x0C

// Receiving 
#define REG_COMPARE_COUNT 0x14

// Macro to access registers
#define REG(offset) (*(volatile uint32_t *)((char *)map_base + offset))

int main() {
    int mem_fd;
    void *map_base, *virt_addr;
    volatile uint32_t *ptr;

    // Open physical memory device fike
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        perror("open /dev/mem");
        return -1;
    }

    // Maps 4KB of physical memory starting from AXI base addr
    // Gets a pointer to the AXI registers used used to access AXI
    map_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, BASE_ADDR & ~MAP_MASK);
    if (map_base == MAP_FAILED) {
        perror("mmap");
        close(mem_fd);
        return -1;
    }

    // Set input signals to AXi registers
    uint32_t nonce_start = 0x00000000;
    REG(REG_NONCE_START) = nonce_start;
    REG(REG_TARGET_LO)   = 0x000000FF;
    REG(REG_TARGET_HI)   = 0x00000000;

    // Start the miner - set only 1 control bit 
    REG(REG_CTRL_READY) = 0X01;


    uint32_t compare_data = 0;
    uint32_t compare_count = 0;
    bool valid_found = false;

    // Polling 
    printf("Waiting for valid nonce to be found or max limit to be reached\n");

    while (1) { // checks compare count continuously
        compare_data = REG(REG_COMPARE_COUNT); 

        // Check is valid bit is set to 1 
        valid_found = (compare_data >> 31) & 0x1;

        // Get compare count
        compare_count = compare_data & COUNT_MASK;
        
        // Exit loop is nonce found or compare limit reached
        if (valid_found || compare_count >= MAX_COMPARE_COUNT) {

            uint32_t valid_nonce = nonce_start + compare_count;

            if (valid_found) {
                printf("Valid nonce: 0x%08X\n", valid_nonce);
            } else {
                printf("Max compare limit reached, no valid nonce found.\n");
            }

            break;

        }
        
        usleep(1000); // delays for 1ms 
    }

    munmap(map_base, MAP_SIZE);
    close(mem_fd);
    return 0;
}
