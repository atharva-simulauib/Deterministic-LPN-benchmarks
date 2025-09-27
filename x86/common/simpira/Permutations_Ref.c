/*
   Simpira v2 reference C implementation

   Written in 2016 by Shay Gueron and Nicky Mouha
   
   This file implements all permutations, using AES intrinsics

   To the extent possible under law, the author has dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>

#include "dlpn.h"


__m128i F(__m128i x, int b, int c) {
  __m128i C = _mm_setr_epi32(0x00^c^B, 0x10^c^B, 0x20^c^B, 0x30^c^B);
  __m128i Z = _mm_setr_epi32(0, 0, 0, 0);
  return _mm_aesenc_si128(_mm_aesenc_si128(x,C), Z);
}

__m128i F_last(__m128i x, int b, int c) {
  __m128i C = _mm_setr_epi32(0x00^c^B, 0x10^c^B, 0x20^c^B, 0x30^c^B);
  __m128i Z = _mm_setr_epi32(0, 0, 0, 0);
  return _mm_aesenclast_si128(_mm_aesenc_si128(x,C), Z);
}

__m128i Fi(__m128i x, int b, int c) {
  __m128i Ci = _mm_aesimc_si128(_mm_setr_epi32(0x00^c^B, 0x10^c^B, 0x20^c^B, 0x30^c^B));
  __m128i Z = _mm_setr_epi32(0, 0, 0, 0);
  return _mm_aesdec_si128(_mm_aesdec_si128(x,Ci), Z);
}

__m128i Fi_last(__m128i x, int b, int c) {
  __m128i Ci = _mm_aesimc_si128(_mm_setr_epi32(0x00^c^B, 0x10^c^B, 0x20^c^B, 0x30^c^B));
  __m128i Z = _mm_setr_epi32(0, 0, 0, 0);
  return _mm_aesdeclast_si128(_mm_aesdec_si128(x,Ci), Z);
}

/*
void perm1(__m128i x[B]) {
  int R = 6;
  int c;

  for (c=1; c<R; c++) {
    x[0] = F(x[0],B,c);
  }
  x[0] = F_last(x[0],B,c);
}


void perm6(__m128i x[B]) {
  int R = 15;
  int c = 1;
  int r;

  int s[6] = {0, 1, 2, 5, 4, 3};
  
  for (r=0; r<R; r++) {
    x[s[(r+1)%B]] ^= F(x[s[ r   %B]],B,c++);
    x[s[(r+5)%B]] ^= F(x[s[(r+2)%B]],B,c++);
    x[s[(r+3)%B]] ^= F(x[s[(r+4)%B]],B,c++);
  }
}
*/

void perm8(__m128i x[B]) {
  int R = 18;
  int c = 1;
  int r;

  int s[6] = {0, 1, 6, 5, 4, 3};
  int t[2] = {2, 7};
  
  for (r=0; r<R; r++) {
    x[s[(r+1)%6]] ^= F(x[s[ r   %6]],B,c++);
    x[s[(r+5)%6]] ^= F(x[t[ r   %2]],B,c++);
    x[s[(r+3)%6]] ^= F(x[s[(r+4)%6]],B,c++);
    x[t[(r+1)%2]] ^= F(x[s[(r+2)%6]],B,c++);
  }
}

void smallperm(__m128i x[B]) {
  int R, r, c;
  
  if (B <= 3) {
    R = 6*B+3;
  } else {
    R = 6*B-9;
  }

  c=1;
  
  for (r=0; r<R; r++) {
    x[(r+1)%B] ^= F(x[r%B],B,c++);
  
    if (B == 4) {
      x[(r+3)%B] ^= F(x[(r+2)%B],B,c++);
    }
  }
    
}

void doubleF(__m128i x[B], int r, int k) {
  if (r%2) {
    x[r  ] ^= F(x[r+1],B,2*k+1);
    x[r+1] ^= F(x[r  ],B,2*k+2);
  } else {
    x[r+1] ^= F(x[r  ],B,2*k+1);
    x[r  ] ^= F(x[r+1],B,2*k+2);
  }
}

void bigperm(__m128i x[B]) {
  int j, r;
  int k = 0;
  int D = (B/2)*2;  
  
  for (j=0; j<3; j++) {
    if (D != B) {
      doubleF(x,B-2,k++);
    }

    for (r=0; r<D-1; r++) {
      doubleF(x,r,k++);
      if (r != D-r-2) {
        doubleF(x,D-r-2,k++);
      }
    }

    if (D != B) {
      doubleF(x,B-2,k++);
    }
  }
}

/*
void invperm1(__m128i x[B]) {
  int R = 6;
  int c;
  
  for (c=R; c>1; c--) {
    x[0] = Fi(x[0],B,c);
  }
  x[0] = Fi_last(x[0],B,c);
}

void invperm6(__m128i x[B]) {
  int R = 15;
  int c = 45;
  int r;

  int s[6] = {0, 1, 2, 5, 4, 3};
  
  for (r=R-1; r>=0; r--) {
    x[s[(r+3)%B]] ^= F(x[s[(r+4)%B]],B,c--);
    x[s[(r+5)%B]] ^= F(x[s[(r+2)%B]],B,c--);
    x[s[(r+1)%B]] ^= F(x[s[ r   %B]],B,c--);
  }
}

void invperm8(__m128i x[B]) {
  int R = 18;
  int c = 72;
  int r;

  int s[6] = {0, 1, 6, 5, 4, 3};
  int t[2] = {2, 7};
  
  for (r=R-1; r>=0; r--) {
    x[t[(r+1)%2]] ^= F(x[s[(r+2)%6]],B,c--);
    x[s[(r+3)%6]] ^= F(x[s[(r+4)%6]],B,c--);
    x[s[(r+5)%6]] ^= F(x[t[ r   %2]],B,c--);
    x[s[(r+1)%6]] ^= F(x[s[ r   %6]],B,c--);
  }
}

void invsmallperm(__m128i x[B]) {
  int R, r, c;
  
  if (B <= 3) {
    R = 6*B+3;
    c = R;
  } else {
    R = 6*B-9;
    c = 2*R;
  }
  
  for (r=R-1; r>=0; r--) {
    if (B == 4) {
      x[(r+3)%B] ^= F(x[(r+2)%B],B,c--);
    }

    x[(r+1)%B] ^= F(x[r%B],B,c--);
  }
}

void invdoubleF(__m128i x[B], int r, int k) {
  if (r%2) {
    x[r+1] ^= F(x[r  ],B,2*k+2);
    x[r  ] ^= F(x[r+1],B,2*k+1);
  } else {
    x[r  ] ^= F(x[r+1],B,2*k+2);
    x[r+1] ^= F(x[r  ],B,2*k+1);
  }
}

void invbigperm(__m128i x[B]) {
  int j, r;
  int k = 6*B-10;
  int D = (B/2)*2;  
  
  for (j=0; j<3; j++) {
    if (D != B) {
      invdoubleF(x,B-2,k--);
    }

    for (r=D-2; r>=0; r--) {
      if (r != D-r-2) {
        invdoubleF(x,D-r-2,k--);
      }
      invdoubleF(x,r,k--);
    }

    if (D != B) {
      invdoubleF(x,B-2,k--);
    }
  }
}
*/

void print(__m128i x[B]) {
  int i;
  uint32_t *v = (uint32_t*) &x[0];
  
  for (i=0; i<B; i++) {
    printf("%08x %08x %08x %08x ", v[4*i], v[4*i+1], v[4*i+2], v[4*i+3]);
  }
  printf("\n");
}

