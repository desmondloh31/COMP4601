#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include <ap_int.h>
#include <hls_stream.h>
#include "sha3_hls.h"


void sha3_keccakf_hls(uint64_t state[25])
{
    #pragma HLS INTERFACE mode=bram port=state
    #pragma HLS ARRAY_PARTITION variable=state complete dim=1
    
    // Constants - marked static for ROM inference
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
    #pragma HLS ARRAY_PARTITION variable=keccakf_rndc complete dim=1

    static const int keccakf_rotc[24] = {
        1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
        27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
    };
    #pragma HLS ARRAY_PARTITION variable=keccakf_rotc complete dim=1

    static const int keccakf_piln[24] = {
        10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
        15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
    };
    #pragma HLS ARRAY_PARTITION variable=keccakf_piln complete dim=1

    // Local variables
    uint64_t t, bc[5];
    #pragma HLS ARRAY_PARTITION variable=bc complete dim=1

    // Round function with pipeline optimization
    ROUND_LOOP: for (int r = 0; r < KECCAKF_ROUNDS; r++) {
        #pragma HLS PIPELINE off  // Pipeline individual steps instead
        
        // Theta step - compute column parities
        THETA_PARITY: for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            bc[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];
        }

        // Theta step - apply transformation
        THETA_TRANSFORM: for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
            // Unroll the inner loop completely
            state[i]      ^= t;
            state[i + 5]  ^= t;
            state[i + 10] ^= t;
            state[i + 15] ^= t;
            state[i + 20] ^= t;
        }

        // Rho and Pi steps combined
        t = state[1];
        RHO_PI: for (int i = 0; i < 24; i++) {
            #pragma HLS PIPELINE II=1
            int j = keccakf_piln[i];
            bc[0] = state[j];
            state[j] = ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        // Chi step - process 5 rows in parallel
        CHI_ROW_0: for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            bc[i] = state[i];
        }
        for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            state[i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        CHI_ROW_1: for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            bc[i] = state[i + 5];
        }
        for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            state[i + 5] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        CHI_ROW_2: for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            bc[i] = state[i + 10];
        }
        for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            state[i + 10] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        CHI_ROW_3: for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            bc[i] = state[i + 15];
        }
        for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            state[i + 15] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        CHI_ROW_4: for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            bc[i] = state[i + 20];
        }
        for (int i = 0; i < 5; i++) {
            #pragma HLS UNROLL
            state[i + 20] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        // Iota step
        state[0] ^= keccakf_rndc[r];
    }
}


void bytes_to_words(const uint8_t bytes[200], uint64_t words[25])
{
    #pragma HLS INLINE
    #pragma HLS ARRAY_PARTITION variable=words complete dim=1
    
    CONVERT_LOOP: for (int i = 0; i < 25; i++) {
        #pragma HLS UNROLL
        int byte_offset = i * 8;
        words[i] = ((uint64_t)bytes[byte_offset])     |
                   ((uint64_t)bytes[byte_offset + 1] << 8)  |
                   ((uint64_t)bytes[byte_offset + 2] << 16) |
                   ((uint64_t)bytes[byte_offset + 3] << 24) |
                   ((uint64_t)bytes[byte_offset + 4] << 32) |
                   ((uint64_t)bytes[byte_offset + 5] << 40) |
                   ((uint64_t)bytes[byte_offset + 6] << 48) |
                   ((uint64_t)bytes[byte_offset + 7] << 56);
    }
}


void words_to_bytes(const uint64_t words[25], uint8_t bytes[200])
{
    #pragma HLS INLINE
    #pragma HLS ARRAY_PARTITION variable=words complete dim=1
    
    CONVERT_LOOP: for (int i = 0; i < 25; i++) {
        #pragma HLS UNROLL
        int byte_offset = i * 8;
        uint64_t word = words[i];
        bytes[byte_offset]     = word & 0xFF;
        bytes[byte_offset + 1] = (word >> 8) & 0xFF;
        bytes[byte_offset + 2] = (word >> 16) & 0xFF;
        bytes[byte_offset + 3] = (word >> 24) & 0xFF;
        bytes[byte_offset + 4] = (word >> 32) & 0xFF;
        bytes[byte_offset + 5] = (word >> 40) & 0xFF;
        bytes[byte_offset + 6] = (word >> 48) & 0xFF;
        bytes[byte_offset + 7] = (word >> 56) & 0xFF;
    }
}


void sha3_init_hls(sha3_ctx_hls_t *ctx, int mdlen)
{
    #pragma HLS INTERFACE mode=s_axilite port=ctx
    #pragma HLS INTERFACE mode=s_axilite port=mdlen
    #pragma HLS INTERFACE mode=s_axilite port=return
    
    // Initialize state
    INIT_STATE: for (int i = 0; i < 25; i++) {
        #pragma HLS UNROLL
        ctx->state[i] = 0;
    }
    
    // Initialize buffer
    INIT_BUFFER: for (int i = 0; i < 200; i++) {
        #pragma HLS UNROLL factor=8
        ctx->buffer[i] = 0;
    }
    
    ctx->mdlen = mdlen;
    ctx->rsiz = 200 - 2 * mdlen;
    ctx->pt = 0;
    ctx->buffer_valid = 0;
}


void sha3_process_block_hls(
    sha3_ctx_hls_t *ctx,
    const uint8_t message_block[MAX_MESSAGE_SIZE],
    int block_size,
    int is_final_block
)
{
    #pragma HLS INTERFACE mode=s_axilite port=ctx
    #pragma HLS INTERFACE mode=bram port=message_block
    #pragma HLS INTERFACE mode=s_axilite port=block_size
    #pragma HLS INTERFACE mode=s_axilite port=is_final_block
    #pragma HLS INTERFACE mode=s_axilite port=return
    
    // Process input bytes
    PROCESS_LOOP: for (int i = 0; i < block_size; i++) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_MESSAGE_SIZE
        
        if (i < MAX_MESSAGE_SIZE) {  // Bounds check for HLS
            ctx->buffer[ctx->pt] ^= message_block[i];
            ctx->pt++;
            
            // Process full rate-sized blocks
            if (ctx->pt >= ctx->rsiz) {
                // Convert buffer to state words and process
                bytes_to_words(ctx->buffer, ctx->state);
                sha3_keccakf_hls(ctx->state);
                words_to_bytes(ctx->state, ctx->buffer);
                ctx->pt = 0;
            }
        }
    }
    
    // Apply final padding if this is the last block
    if (is_final_block) {
        ctx->buffer[ctx->pt] ^= 0x06;
        ctx->buffer[ctx->rsiz - 1] ^= 0x80;
        
        // Final round
        bytes_to_words(ctx->buffer, ctx->state);
        sha3_keccakf_hls(ctx->state);
        words_to_bytes(ctx->state, ctx->buffer);
    }
}


void sha3_get_hash_hls(
    const sha3_ctx_hls_t *ctx,
    uint8_t hash_output[MAX_OUTPUT_SIZE],
    int output_length
)
{
    #pragma HLS INTERFACE mode=s_axilite port=ctx
    #pragma HLS INTERFACE mode=bram port=hash_output
    #pragma HLS INTERFACE mode=s_axilite port=output_length
    #pragma HLS INTERFACE mode=s_axilite port=return
    
    // Copy hash output with bounds checking
    OUTPUT_LOOP: for (int i = 0; i < MAX_OUTPUT_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        if (i < output_length && i < MAX_OUTPUT_SIZE) {
            hash_output[i] = ctx->buffer[i];
        } else {
            hash_output[i] = 0;  // Pad with zeros
        }
    }
}


void sha3_compute_hls(
    const uint8_t input_message[MAX_MESSAGE_SIZE],
    int message_length,
    uint8_t output_hash[MAX_OUTPUT_SIZE],
    int hash_length
)
{
    #pragma HLS INTERFACE mode=bram port=input_message
    #pragma HLS INTERFACE mode=s_axilite port=message_length
    #pragma HLS INTERFACE mode=bram port=output_hash
    #pragma HLS INTERFACE mode=s_axilite port=hash_length
    #pragma HLS INTERFACE mode=s_axilite port=return
    
    #pragma HLS DATAFLOW
    
    // Local context
    sha3_ctx_hls_t ctx;
    #pragma HLS ARRAY_PARTITION variable=ctx.state complete dim=1
    
    // Initialize
    sha3_init_hls(&ctx, hash_length);
    
    // Process message (single block for simplicity)
    sha3_process_block_hls(&ctx, input_message, message_length, 1);
    
    // Extract hash
    sha3_get_hash_hls(&ctx, output_hash, hash_length);
}


void sha3_streaming_hls(
    const uint8_t input_stream[MAX_MESSAGE_SIZE],
    uint8_t output_stream[MAX_OUTPUT_SIZE],
    const int block_lengths[256],  // Fixed-size array of block lengths
    int num_blocks,
    int hash_length
)
{
    #pragma HLS INTERFACE mode=bram port=input_stream
    #pragma HLS INTERFACE mode=bram port=output_stream
    #pragma HLS INTERFACE mode=bram port=block_lengths
    #pragma HLS INTERFACE mode=s_axilite port=num_blocks
    #pragma HLS INTERFACE mode=s_axilite port=hash_length
    #pragma HLS INTERFACE mode=s_axilite port=return
    
    int input_offset = 0;
    int output_offset = 0;
    
    // Process multiple blocks
    STREAM_LOOP: for (int block = 0; block < num_blocks && block < 256; block++) {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=256
        
        sha3_ctx_hls_t ctx;
        int current_length = block_lengths[block];
        
        if (current_length > 0 && input_offset + current_length <= MAX_MESSAGE_SIZE) {
            sha3_init_hls(&ctx, hash_length);
            
            // Process current block
            uint8_t temp_block[MAX_MESSAGE_SIZE];
            #pragma HLS ARRAY_PARTITION variable=temp_block cyclic factor=8
            
            COPY_LOOP: for (int i = 0; i < current_length; i++) {
                #pragma HLS PIPELINE II=1
                temp_block[i] = input_stream[input_offset + i];
            }
            
            sha3_process_block_hls(&ctx, temp_block, current_length, 1);
            
            // Output hash
            uint8_t temp_hash[MAX_OUTPUT_SIZE];
            sha3_get_hash_hls(&ctx, temp_hash, hash_length);
            
            OUTPUT_COPY: for (int i = 0; i < hash_length; i++) {
                #pragma HLS PIPELINE II=1
                if (output_offset + i < MAX_OUTPUT_SIZE) {
                    output_stream[output_offset + i] = temp_hash[i];
                }
            }
            
            input_offset += current_length;
            output_offset += hash_length;
        }
    }
}

