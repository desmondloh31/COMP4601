#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#define TEST_ADDR 0xA0010000UL
#define PAGE_SIZE 4096UL

int test_devmem_access() {
    int fd;
    void *map_base;
    
    printf("=== Testing /dev/mem access ===\n");
    
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        printf("Failed to open /dev/mem: %s\n", strerror(errno));
        return -1;
    }
    printf("✓ Successfully opened /dev/mem\n");
    
    // Test with page-aligned address
    unsigned long aligned_addr = TEST_ADDR & ~(PAGE_SIZE - 1);
    printf("Original address: 0x%lX\n", TEST_ADDR);
    printf("Aligned address:  0x%lX\n", aligned_addr);
    
    map_base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, aligned_addr);
    if (map_base == MAP_FAILED) {
        printf("✗ mmap failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    
    printf("✓ Successfully mapped memory at %p\n", map_base);
    
    // Try to read from the mapped memory
    volatile uint32_t *reg_ptr = (volatile uint32_t *)((char *)map_base + (TEST_ADDR - aligned_addr));
    
    printf("Attempting to read from mapped memory...\n");
    uint32_t value = *reg_ptr;
    printf("✓ Read value: 0x%08X\n", value);
    
    printf("Attempting to write to mapped memory...\n");
    *reg_ptr = 0x12345678;
    uint32_t readback = *reg_ptr;
    printf("✓ Write/readback test: wrote 0x12345678, read 0x%08X\n", readback);
    
    munmap(map_base, PAGE_SIZE);
    close(fd);
    printf("✓ Cleanup completed\n");
    return 0;
}

void check_system_info() {
    printf("\n=== System Information ===\n");
    
    system("uname -a");
    printf("\nPage size: %ld bytes\n", sysconf(_SC_PAGE_SIZE));
    
    printf("\nChecking for FPGA/AXI devices:\n");
    system("ls -la /sys/bus/platform/devices/ | grep -i axi || echo 'No AXI devices found'");
    system("ls -la /sys/bus/platform/devices/ | grep -i fpga || echo 'No FPGA devices found'");
    
    printf("\nChecking memory map around target address:\n");
    system("sudo cat /proc/iomem | grep -i 'a00\\|axi\\|fpga' || echo 'No relevant entries found'");
    
    printf("\nChecking for UIO devices:\n");
    system("ls -la /dev/uio* 2>/dev/null || echo 'No UIO devices found'");
    
    printf("\nChecking loaded kernel modules:\n");
    system("lsmod | grep -i 'uio\\|fpga\\|xilinx' || echo 'No relevant modules loaded'");
}

int test_alternative_addresses() {
    printf("\n=== Testing Alternative Addresses ===\n");
    
    // Common FPGA/AXI base addresses to test
    unsigned long test_addresses[] = {
        0x40000000UL,  // Common AXI base
        0x41000000UL,  // Another common AXI base
        0x43C00000UL,  // Common AXI-Lite base
        0x80000000UL,  // High memory base
        0xA0000000UL,  // Your base region
        0xA0010000UL,  // Your specific address
        0
    };
    
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        printf("Cannot open /dev/mem for testing\n");
        return -1;
    }
    
    for (int i = 0; test_addresses[i] != 0; i++) {
        printf("Testing address 0x%08lX: ", test_addresses[i]);
        
        void *map_base = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, 
                             test_addresses[i] & ~(PAGE_SIZE - 1));
        
        if (map_base == MAP_FAILED) {
            printf("✗ Failed (%s)\n", strerror(errno));
        } else {
            printf("✓ Success\n");
            munmap(map_base, PAGE_SIZE);
        }
    }
    
    close(fd);
    return 0;
}

int main() {
    printf("Hardware Register Access Diagnostic Tool\n");
    printf("========================================\n");
    
    // Check if we're running as root
    if (getuid() != 0) {
        printf("WARNING: Not running as root. Some tests may fail.\n");
    }
    
    check_system_info();
    test_alternative_addresses();
    test_devmem_access();
    
    printf("\n=== Recommendations ===\n");
    printf("If mmap continues to fail:\n");
    printf("1. Verify your FPGA bitstream is loaded correctly\n");
    printf("2. Check if your address 0xA0010000 is in the device tree\n");
    printf("3. Consider using a different base address (try 0x43C00000)\n");
    printf("4. Add UIO driver support to your device tree\n");
    printf("5. Check Vivado address editor for actual assigned addresses\n");
    
    return 0;
}
