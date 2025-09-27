#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <arm_neon.h>
#include "core.h"

int main() {
    // Hardcoded random 128-bit values (16 bytes each)
    uint8_t x_bytes[16] = {0x34, 0x12, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
                           0x01, 0x23, 0x45, 0x67, 0x89, 0xfe, 0xdc, 0xba};
    uint8_t rk_bytes[16] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
                            0x0f, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87};

    // Load them into NEON vectors
    uint8x16_t x = vld1q_u8(x_bytes);  // Load x_bytes into a NEON vector
    uint8x16_t rk = vld1q_u8(rk_bytes); // Load rk_bytes into a NEON vector

    // Print the input blocks
    print_vector("Input x", x);
    print_vector("Round Key rk", rk);

    // Call your AES-related function (assuming `at_sr_mc_ark` exists)
    uint8x16_t result = at_sr_mc_ark(x, rk);

    // Print the resulting vector
    print_vector("Resulting Block", result);

    return 0;
}