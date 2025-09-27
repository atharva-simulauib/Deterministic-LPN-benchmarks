#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <x86intrin.h>
#include <time.h>
#include <math.h>

#include "dlpn.h"

//------------------------------------------------------------------------------------------------------------------------------------------------
// AES key-schedule* as described in https://www.intel.com/content/dam/doc/white-paper/advanced-encryption-standard-new-instructions-set-paper.pdf

__m128i AES128_ROUND_KEY_ASSIST(__m128i tmp1, __m128i tmp2){
  // tmp1 stores the previous round key
  // tmp2 is the SubWord o RotWord + RConn applied to first word of previous key
  __m128i tmp3;
  tmp2 = _mm_shuffle_epi32(tmp2, 0xff);       // 0xff = 11 11 11 11 selects 4th 32-bit value for each place
  tmp3 = _mm_slli_si128(tmp1, 0x4);
  tmp1 = _mm_xor_si128(tmp1,tmp3);
  tmp3 = _mm_slli_si128(tmp1, 0x4);
  tmp1 = _mm_xor_si128(tmp1,tmp3);
  tmp3 = _mm_slli_si128(tmp1, 0x4);
  tmp1 = _mm_xor_si128(tmp1,tmp3);
  tmp1 = _mm_xor_si128(tmp1,tmp2);
}

void AES_128_KEY_EXPANSION(__m128i key_schedule[10], __m128i k){
  __m128i tmp1, tmp2;
  tmp1 = (__m128i) k ;
  key_schedule[0] = tmp1;

  tmp2 = _mm_aeskeygenassist_si128(tmp1,0x1);          
  tmp1 = AES128_ROUND_KEY_ASSIST(tmp1,tmp2);
  key_schedule[1] = tmp1;

  tmp2 = _mm_aeskeygenassist_si128(tmp1,0x1);          
  tmp1 = AES128_ROUND_KEY_ASSIST(tmp1,tmp2);
  key_schedule[2] = tmp1;

  tmp2 = _mm_aeskeygenassist_si128(tmp1,0x1);          
  tmp1 = AES128_ROUND_KEY_ASSIST(tmp1,tmp2);
  key_schedule[3] = tmp1;

  tmp2 = _mm_aeskeygenassist_si128(tmp1,0x1);          
  tmp1 = AES128_ROUND_KEY_ASSIST(tmp1,tmp2);
  key_schedule[4] = tmp1;
  
  tmp2 = _mm_aeskeygenassist_si128 (tmp1,0x10);
  tmp1 = AES128_ROUND_KEY_ASSIST(tmp1,tmp2);
  key_schedule[5] = tmp1;

  tmp2 = _mm_aeskeygenassist_si128 (tmp1,0x10);
  tmp1 = AES128_ROUND_KEY_ASSIST(tmp1,tmp2);
  key_schedule[6] = tmp1;

  tmp2 = _mm_aeskeygenassist_si128 (tmp1,0x10);
  tmp1 = AES128_ROUND_KEY_ASSIST(tmp1,tmp2);
  key_schedule[7] = tmp1;

  tmp2 = _mm_aeskeygenassist_si128 (tmp1,0x10);
  tmp1 = AES128_ROUND_KEY_ASSIST(tmp1,tmp2);
  key_schedule[8] = tmp1;
  
  tmp2 = _mm_aeskeygenassist_si128 (tmp1,0x10);
  tmp1 = AES128_ROUND_KEY_ASSIST(tmp1,tmp2);
  key_schedule[9] = tmp1;
}

__m128i AES_FOUR_ROUNDS(__m128i x, __m128i* KS){
    x = _mm_xor_si128(x, KS[0]); 
    x = _mm_aesenc_si128(x, KS[1]);
    x = _mm_aesenc_si128(x, KS[2]);
    x = _mm_aesenc_si128(x, KS[3]);
    x = _mm_aesenclast_si128(x, KS[4]);

  return x;
}

__m128i LightMAC_AES4_s8(uint8_t input[], __m128i KS[10]){
  /* LightMAC construction using AES_FOUR_ROUNDS as the block cipher 
  For a block cipher operating on input of length n-bits we choose a parameter s for LightMAC 
  and the input for block cipher is (n-s) bits of given input concatenated with i_s which is s-bit representation of i
  Here we have s = 8. So we parse the input as bytes. Block cipher input is 15 bytes of given input concatenated with 1 byte of i_s 
  __m128i* Key_Schedule;*/
  __m128i res;
  
  for (uint8_t i = 0; i < T2; i++){
    // input for AES 4 will be byte representation of i concatenated with 15 bytes from LPN input
    uint8_t tmpData[16];
    tmpData[0] = i; 
    for (int j = 1; j < 16; j++)
    {
      tmpData[16-j] = input[ 15*i + j];
    }
    
    __m128i m = _mm_loadu_si128( (__m128i*) & tmpData );
    res = _mm_xor_si128( res, AES_FOUR_ROUNDS(m, KS )) ;
  }
  res = AES_FOUR_ROUNDS(res, &KS[4]);

  return res;

}
