/*
   Simpira v2 reference C implementation

   Written in 2016 by Shay Gueron and Nicky Mouha
   
   This file implements only the 1024-bit permutation, using AES intrinsics

   To the extent possible under law, the author has dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>

#define B 8
#define ROUNDS (3*6)

#define C0(i) _mm_setr_epi32(0x00^(4*i+1)^B, 0x10^(4*i+1)^B, 0x20^(4*i+1)^B, 0x30^(4*i+1)^B)
#define C1(i) _mm_setr_epi32(0x00^(4*i+2)^B, 0x10^(4*i+2)^B, 0x20^(4*i+2)^B, 0x30^(4*i+2)^B)
#define C2(i) _mm_setr_epi32(0x00^(4*i+3)^B, 0x10^(4*i+3)^B, 0x20^(4*i+3)^B, 0x30^(4*i+3)^B)
#define C3(i) _mm_setr_epi32(0x00^(4*i+4)^B, 0x10^(4*i+4)^B, 0x20^(4*i+4)^B, 0x30^(4*i+4)^B)

#define R(s,t,u,v,w,x,y,z,i) do { \
  t = _mm_aesenc_si128(_mm_aesenc_si128(s,C0(i)), t); \
  v = _mm_aesenc_si128(_mm_aesenc_si128(u,C1(i)), v); \
  x = _mm_aesenc_si128(_mm_aesenc_si128(w,C2(i)), x); \
  z = _mm_aesenc_si128(_mm_aesenc_si128(y,C3(i)), z); \
} while (0)

void perm(__m128i b[B]) {
  /* Type-II Generalized Feistel, Suzaki-Minematsu shuffle (3,0,7,4,5,6,1,2) */
  
  R(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], 0);
  R(b[1], b[6], b[7], b[0], b[3], b[4], b[5], b[2], 1);
  R(b[6], b[5], b[2], b[1], b[0], b[3], b[4], b[7], 2);
  R(b[5], b[4], b[7], b[6], b[1], b[0], b[3], b[2], 3);
  R(b[4], b[3], b[2], b[5], b[6], b[1], b[0], b[7], 4);
  R(b[3], b[0], b[7], b[4], b[5], b[6], b[1], b[2], 5);
  R(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], 6);
  R(b[1], b[6], b[7], b[0], b[3], b[4], b[5], b[2], 7);
  R(b[6], b[5], b[2], b[1], b[0], b[3], b[4], b[7], 8);
  R(b[5], b[4], b[7], b[6], b[1], b[0], b[3], b[2], 9);
  R(b[4], b[3], b[2], b[5], b[6], b[1], b[0], b[7],10);
  R(b[3], b[0], b[7], b[4], b[5], b[6], b[1], b[2],11);
  R(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],12);
  R(b[1], b[6], b[7], b[0], b[3], b[4], b[5], b[2],13);
  R(b[6], b[5], b[2], b[1], b[0], b[3], b[4], b[7],14);
  R(b[5], b[4], b[7], b[6], b[1], b[0], b[3], b[2],15);
  R(b[4], b[3], b[2], b[5], b[6], b[1], b[0], b[7],16);
  R(b[3], b[0], b[7], b[4], b[5], b[6], b[1], b[2],17);

}

void invperm(__m128i b[B]) {
  /* Type-II Generalized Feistel, Suzaki-Minematsu shuffle (3,0,7,4,5,6,1,2) */

  R(b[3], b[0], b[7], b[4], b[5], b[6], b[1], b[2],17);
  R(b[4], b[3], b[2], b[5], b[6], b[1], b[0], b[7],16);
  R(b[5], b[4], b[7], b[6], b[1], b[0], b[3], b[2],15);
  R(b[6], b[5], b[2], b[1], b[0], b[3], b[4], b[7],14);
  R(b[1], b[6], b[7], b[0], b[3], b[4], b[5], b[2],13);
  R(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],12);
  R(b[3], b[0], b[7], b[4], b[5], b[6], b[1], b[2],11);
  R(b[4], b[3], b[2], b[5], b[6], b[1], b[0], b[7],10);
  R(b[5], b[4], b[7], b[6], b[1], b[0], b[3], b[2], 9);
  R(b[6], b[5], b[2], b[1], b[0], b[3], b[4], b[7], 8);
  R(b[1], b[6], b[7], b[0], b[3], b[4], b[5], b[2], 7);
  R(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], 6);
  R(b[3], b[0], b[7], b[4], b[5], b[6], b[1], b[2], 5);
  R(b[4], b[3], b[2], b[5], b[6], b[1], b[0], b[7], 4);
  R(b[5], b[4], b[7], b[6], b[1], b[0], b[3], b[2], 3);
  R(b[6], b[5], b[2], b[1], b[0], b[3], b[4], b[7], 2);
  R(b[1], b[6], b[7], b[0], b[3], b[4], b[5], b[2], 1);
  R(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], 0);
}

void print(__m128i x[B]) {
  int i;
  uint32_t *v = (uint32_t*) &x[0];
  
  for (i=0; i<B; i++) {
    printf("%08x %08x %08x %08x ", v[4*i], v[4*i+1], v[4*i+2], v[4*i+3]);
  }
  printf("\n");
}

int main(int argc, char *argv[]) {

  __m128i x[B] = {{0}};

  printf("x: ");
  print(x);

  perm(x);

  printf("y: ");
  print(x);

  invperm(x);
  
  printf("x: ");
  print(x);

  return 0;
}
