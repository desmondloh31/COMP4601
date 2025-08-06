// HLS-Friendly SHA-3 Implementation
// Addresses HLS synthesis issues with unions, arbitrary arrays, and dynamic indexing

#ifndef SHA3_HLS_H
#define SHA3_HLS_H

#include <stdint.h>
#include <stddef.h>

// HLS-specific includes
#ifdef __SYNTHESIS__
#include "ap_int.h"
#endif

#ifndef KECCAKF_ROUNDS
#define KECCAKF_ROUNDS 24
#endif

#ifndef ROTL64
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

// Maximum supported message size (adjust based on your needs)
#define MAX_MESSAGE_SIZE 512
#define MAX_OUTPUT_SIZE 64

// HLS-friendly context structure - NO UNION
typedef struct {
    uint64_t state[25];          // 64-bit state words (replaces union)
    uint8_t  buffer[200];        // Separate byte buffer for input processing
    int      pt;                 // Buffer pointer
    int      rsiz;              // Rate size
    int      mdlen;             // Message digest length
    int      buffer_valid;       // Indicates if buffer contains valid data
} sha3_ctx_hls_t;

// Fixed-size message processing (HLS-friendly)
void sha3_keccakf_hls(uint64_t state[25]);


// Helper function to convert bytes to 64-bit words (explicit, no union)
void bytes_to_words(const uint8_t bytes[200], uint64_t words[25]);


// Helper function to convert 64-bit words to bytes (explicit, no union)
void words_to_bytes(const uint64_t words[25], uint8_t bytes[200]);


// HLS-friendly initialization
void sha3_init_hls(sha3_ctx_hls_t *ctx, int mdlen);


// Fixed-size message processing (avoids arbitrary-length arrays)
void sha3_process_block_hls(
    sha3_ctx_hls_t *ctx,
    const uint8_t message_block[MAX_MESSAGE_SIZE],
    int block_size,
    int is_final_block
);


// Fixed-size output generation
void sha3_get_hash_hls(
    const sha3_ctx_hls_t *ctx,
    uint8_t hash_output[MAX_OUTPUT_SIZE],
    int output_length
);


// Complete SHA-3 computation with fixed-size inputs/outputs
void sha3_compute_hls(
    const uint8_t input_message[MAX_MESSAGE_SIZE],
    int message_length,
    uint8_t output_hash[MAX_OUTPUT_SIZE],
    int hash_length
);


// Streaming version for high throughput
void sha3_streaming_hls(
    const uint8_t input_stream[MAX_MESSAGE_SIZE],
    uint8_t output_stream[MAX_OUTPUT_SIZE],
    const int block_lengths[256],  // Fixed-size array of block lengths
    int num_blocks,
    int hash_length
);


#endif // SHA3_HLS_H

