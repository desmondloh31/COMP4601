#include <stdio.h>
#include <stdint.h>

// Control register addresses (adjust for your memory map)
#define MINER_BASE_ADDR 0x43C00000
#define REG_START       (MINER_BASE_ADDR + 0x00)
#define REG_STATUS      (MINER_BASE_ADDR + 0x08)
#define REG_NONCE       (MINER_BASE_ADDR + 0x0C)
#define REG_TARGET      (MINER_BASE_ADDR + 0x10)
#define REG_RESULT      (MINER_BASE_ADDR + 0x14)

void start_mining(uint32_t initial_nonce, uint32_t target) {
    *(volatile uint32_t*)REG_NONCE = initial_nonce;
    *(volatile uint32_t*)REG_TARGET = target;
    *(volatile uint32_t*)REG_START = 1;
}

uint32_t check_mining_result() {
    uint32_t status = *(volatile uint32_t*)REG_STATUS;
    if (status == 2) {  // Found solution
        return *(volatile uint32_t*)REG_RESULT;
    }
    return 0;  // Still mining or no solution
}

