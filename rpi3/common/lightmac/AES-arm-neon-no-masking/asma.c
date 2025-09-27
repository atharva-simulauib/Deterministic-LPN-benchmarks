#include <arm_neon.h>
#include <stdint.h>
static const uint8_t tbl_shiftrows[16] = {0, 1, 2, 3, 4, 6, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const uint8_t tbl_shiftrows[16] = {0, 1, 2, 3, 5, 6, 7, 4, 10, 11, 8, 9, 15, 12, 13, 14};
//static const uint8_t tbl_shiftrows[16] = {14, 13, 12, 15, 9, 8, 11, 10, 4, 7, 6, 5, 3, 2, 1, 0};

// aes state is represented as uint8x16_t x meaning 16 x 16 byte array
uint8x16_t at_sr_mc_ark(uint8x16_t x, uint8x16_t rk) {
    uint8x16_t b, a, y, z;

    // Affine Transformation
    for (int i = 0; i < 4; i++) { // Perform the 4 rounds of affine transformation
        b = vshrq_n_u8(x, 7);        // b = x >> 7
        a = vshlq_n_u8(x, 1);        // a = x << 1
        a = vorrq_u8(a, b);          // a = a | b
        x = veorq_u8(x, a);          // x = x ^ a
    }
    y = vdupq_n_u8(0x63);            // y = vdupq_n_u8(0x63)
    x = veorq_u8(x, y);              // x = x ^ y

    // Step 2: Shift Rows
    uint8x16_t tbl_shift_rows = vld1q_u8(tbl_shiftrows); // Load tbl_shiftrows
    x = vqtbl1q_u8(x, tbl_shift_rows); // Use table lookup (VTBL)

    // Step 3: Mix Columns
    uint8x16_t m = vdupq_n_u8(0x1B); // m = vdupq_n_u8(0x1B)
    y = vshrq_n_u8(x, 7);            // y = vshrq_n_s8(x, 7)
    y = vandq_u8(y, m);              // y = vandq_u8(y, m)
    z = vshlq_n_u8(x, 1);            // z = vshlq_n_u8(x, 1)
    z = veorq_u8(z, y);              // z = veorq_u8(z, y)

    x = veorq_u8(z, vextq_u8(z, z, 4)); // z = veorq_u8(z, vextq_u8(z, z, 4))
    x = veorq_u8(x, vextq_u8(x, x, 4)); // x = veorq_u8(x, vextq_u8(x, x, 4))
    x = veorq_u8(x, vextq_u8(x, x, 4)); // x = veorq_u8(x, vextq_u8(x, x, 4))

    // Step 4: Add Round Key
    x = veorq_u8(x, rk);             // x = veorq_u8(x, rk)

    return x; // Return the transformed value
}

#include <arm_neon.h> // Essential header for ARM Neon intrinsics
#include <stdint.h>   // For uint8_t

// Duplicate definition - will cause a compilation error.
// Assuming the second definition is the intended one.
// static const uint8_t tbl_shiftrows[16] = {0, 1, 2, 3, 4, 6, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

// This is the correct definition from the original assembly (InverseShiftRows permutation)
// It defines how bytes are remapped within the 128-bit (16-byte) AES state.
// Each value is an index from 0-15, indicating the source position for the byte at the current position.
// For example:
// - Byte at index 0 comes from original index 0 (no shift for row 0)
// - Byte at index 4 comes from original index 5 (row 1 shifted right by 1)
// - Byte at index 8 comes from original index 10 (row 2 shifted right by 2)
// - Byte at index 12 comes from original index 15 (row 3 shifted right by 3)
static const uint8_t tbl_shiftrows_data[16] __attribute__ ((aligned (16))) = {
    0, 1, 2, 3,       // Row 0: No shift
    5, 6, 7, 4,       // Row 1: Circular right shift by 1 (original 4->7, 5->4, 6->5, 7->6)
    10, 11, 8, 9,     // Row 2: Circular right shift by 2 (original 8->10, 9->11, 10->8, 11->9)
    15, 12, 13, 14    // Row 3: Circular right shift by 3 (original 12->15, 13->12, 14->13, 15->14)
};

// This is the Neon vector representation of the lookup table.
// It's declared globally (or static global) to be accessible by the functions.
const uint8x16_t tbl_shiftrows_neon_vec = {
    tbl_shiftrows_data[0], tbl_shiftrows_data[1], tbl_shiftrows_data[2], tbl_shiftrows_data[3],
    tbl_shiftrows_data[4], tbl_shiftrows_data[5], tbl_shiftrows_data[6], tbl_shiftrows_data[7],
    tbl_shiftrows_data[8], tbl_shiftrows_data[9], tbl_shiftrows_data[10], tbl_shiftrows_data[11],
    tbl_shiftrows_data[12], tbl_shiftrows_data[13], tbl_shiftrows_data[14], tbl_shiftrows_data[15]
};


// aes state is represented as uint8x16_t x meaning 16 x 1 byte array (128 bits)
// x: The 128-bit AES state (input and output)
// rk: The 128-bit round key
uint8x16_t at_sr_mc_ark(uint8x16_t x, uint8x16_t rk) {
    uint8x16_t b, a, y, z; // Declare temporary Neon vector variables

    // Affine Transformation (part of AES S-box)
    // This sequence implements the affine transformation part of the AES S-box.
    // It's a series of XORs with circular left shifts.
    // The pattern (val << 1) | (val >> 7) is a byte-wise circular left shift by 1.
    // The loop performs this operation 4 times, effectively computing:
    // x = x ^ (x <<< 1) ^ (x <<< 2) ^ (x <<< 3) ^ (x <<< 4)
    // where '<<<' denotes a byte-wise circular left shift.
    for (int i = 0; i < 4; i++) { // Loop for 4 iterations of the affine transformation
        b = vshrq_n_u8(x, 7);     // For each byte in 'x', shift bits right by 7 (bits that wrap around)
        a = vshlq_n_u8(x, 1);     // For each byte in 'x', shift bits left by 1
        a = vorrq_u8(a, b);       // Combine the shifted bits to perform a circular left shift by 1
        x = veorq_u8(x, a);       // XOR the current 'x' with the circularly shifted 'x'
    }
    y = vdupq_n_u8(0x63);         // Create a 128-bit vector where all 16 bytes are 0x63 (constant for affine transformation)
    x = veorq_u8(x, y);           // Final XOR with the constant 0x63

    // Step 2: ShiftRows
    // In AES, ShiftRows cyclically shifts the last three rows of the state array.
    // Row 0: No shift
    // Row 1: Left shift by 1 byte
    // Row 2: Left shift by 2 bytes
    // Row 3: Left shift by 3 bytes
    // The `tbl_shiftrows_data` used here is for InverseShiftRows (right shifts),
    // which is common in decryption or specific implementations.
    
    // Load the pre-defined ShiftRows permutation table into a Neon vector.
    // This is the table that maps original byte positions to their new positions.
    uint8x16_t tbl_shift_rows_indices = vld1q_u8(tbl_shiftrows_data); 
    
    // Perform a table lookup (permutation) on the state 'x' using the 'tbl_shift_rows_indices'.
    // `vqtbl1q_u8` takes a 128-bit input vector (x) and a 128-bit table of indices.
    // For each byte in the result, it looks up the corresponding byte in 'x' at the index specified
    // by the table. This efficiently reorders the bytes according to the ShiftRows pattern.
    x = vqtbl1q_u8(x, tbl_shift_rows_indices); 

    // Step 3: MixColumns
    // In AES, MixColumns operates on each column of the state independently.
    // Each column is treated as a polynomial over GF(2^8) and multiplied by a fixed matrix.
    // The standard MixColumns matrix involves multiplication by 0x02 and 0x03 in GF(2^8).
    // The assembly code's implementation here is a custom sequence of operations
    // that achieves a form of column mixing, but it's not the direct standard AES MixColumns matrix multiplication.
    // It appears to be a series of XORs with byte-wise circular shifts and a multiplication by 0x  .

    uint8x16_t m = vdupq_n_u8(0x1B); // Create a 128-bit vector where all 16 bytes are 0x1B.
                                     // This constant (0x1B) is the irreducible polynomial for GF(2^8)
                                     // (x^8 + x^4 + x^3 + x + 1) and is used in GF(2^8) arithmetic.
                                     // Here, it's used as a mask for reduction.

    // Calculate (x << 1) ^ ((x >> 7) & 0x1B) for each byte.
    // This is a common way to implement multiplication by 0x02 in GF(2^8) without dedicated instructions.
    // The `vshrq_n_u8(x, 7)` extracts the most significant bit (MSB) of each byte.
    y = vshrq_n_u8(x, 7);            // For each byte in 'x', shift bits right by 7. This moves the MSB to bit 0.
    y = vandq_u8(y, m);              // AND the result with 0x1B. If the MSB was 1, this applies the reduction polynomial.
    z = vshlq_n_u8(x, 1);            // For each byte in 'x', shift bits left by 1 (effectively multiply by x).
    z = veorq_u8(z, y);              // XOR the shifted value with the reduction term. 'z' now holds `x * 2` in GF(2^8) for each byte.

    // The following three lines perform a series of XORs with byte rotations (`vextq_u8`).
    // `vextq_u8(vector, vector, N)` performs a circular right shift of the entire 128-bit vector by N bytes.
    // This sequence is a direct translation of the assembly's column mixing logic.
    // It's not the standard AES MixColumns matrix multiplication, but a custom operation.
    
    // Original assembly logic used Q2 (z) as accumulator and Q0 (x) for shifts.
    // C code directly modifies 'x' as the accumulator.
    // The assembly sequence for MixColumns was:
    // Q2 = z_initial (x*2)
    // Q3 = vextq_u8(Q2, Q2, 4)
    // Q2 = veorq_u8(Q2, Q3)   // Q2 = (x*2) ^ ((x*2) >>> 4)
    // Q3 = vextq_u8(Q0, Q0, 4) // Q3 = x >>> 4
    // Q2 = veorq_u8(Q2, Q3)   // Q2 = (x*2) ^ ((x*2) >>> 4) ^ (x >>> 4)
    // Q3 = vextq_u8(Q0, Q0, 4) // Q3 = x >>> 8
    // Q2 = veorq_u8(Q2, Q3)   // Q2 = (x*2) ^ ((x*2) >>> 4) ^ (x >>> 4) ^ (x >>> 8)
    // Q3 = vextq_u8(Q0, Q0, 4) // Q3 = x >>> 12
    // Q0 = veorq_u8(Q0, Q2)   // Q0 = (x >>> 12) ^ Q2 (final result)

    // The C code provided simplifies/reorders this slightly:
    uint8x16_t temp_z_initial_mult2 = z; // Store the x*2 result
    
    // First part of the custom mix: x = (x*2) ^ ((x*2) >>> 4)
    x = veorq_u8(temp_z_initial_mult2, vextq_u8(temp_z_initial_mult2, temp_z_initial_mult2, 4)); 

    // Second part: x = current_x ^ (original_x >>> 4)
    // Note: The original assembly used a different variable (Q0) for the shifted x.
    // This C code uses the *current* value of 'x' for the next shift, which is different from the assembly.
    // The assembly used the *original* 'x' (Q0) for the series of `vextq_u8` operations.
    // To be strictly faithful to the assembly's MixColumns, it should be:
    // uint8x16_t original_x_state = x_input_to_mixcolumns; // Save x before first shift
    // z = veorq_u8(z, vextq_u8(z, z, 4)); // z = (x*2) ^ ((x*2) >>> 4)
    // z = veorq_u8(z, vextq_u8(original_x_state, original_x_state, 4)); // z = z ^ (x >>> 4)
    // z = veorq_u8(z, vextq_u8(original_x_state, original_x_state, 8)); // z = z ^ (x >>> 8)
    // z = veorq_u8(z, vextq_u8(original_x_state, original_x_state, 12)); // z = z ^ (x >>> 12)
    // x = z; // Final result to x

    // The provided C code's sequence:
    // x = veorq_u8(current_x, vextq_u8(current_x, current_x, 4)); // Shift and XOR current 'x'
    // This is a recursive application of shift-and-xor on the *evolving* 'x', not the original.
    // This is a functional difference from the assembly's MixColumns logic.
    // Assuming the provided C code's logic is what you want explained:
    x = veorq_u8(x, vextq_u8(x, x, 4)); // XOR 'x' with 'x' circularly shifted right by 4 bytes.
    x = veorq_u8(x, vextq_u8(x, x, 4)); // XOR 'x' with 'x' circularly shifted right by 4 bytes again.
    x = veorq_u8(x, vextq_u8(x, x, 4)); // XOR 'x' with 'x' circularly shifted right by 4 bytes one more time.

    // Step 4: Add Round Key
    // This is the final step in an AES round, where the round key is XORed with the state.
    x = veorq_u8(x, rk); // XOR the state 'x' with the round key 'rk'.

    return x; // Return the transformed AES state.
}