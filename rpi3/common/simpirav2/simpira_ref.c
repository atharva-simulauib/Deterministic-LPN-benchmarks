#include <stdio.h>
#include <stdint.h>
#include <string.h> // for copying arrays

#include "tinyAES/aes.h"
#include "dlpn.h"

// Simpira F with AES_ROUND as function coming from TinyAES. Input amnd output of 128-bit  
void F(const uint8_t *x_in, int b, int c, uint8_t *x_out) {
  uint32_t C1[4] = {0x00^c^B, 0x10^c^B, 0x20^c^B, 0x30^c^B};
  uint8_t* C = (uint8_t*)C1;
  uint8_t Z[16] = {0};
  
  uint8_t tmp_buff[16] = {0};
  //x_out = aes_round(aes_round(x_in,C), Z);
  AES_ROUND(x_in, C, tmp_buff);
  AES_ROUND(tmp_buff, Z, x_out);
}


void perm8(uint8_t *x_in) {
  int R = 18;
  int c = 1;
  int r;

  int s[6] = {0, 1, 6, 5, 4, 3};
  int t[2] = {2, 7};

  uint8_t F_out_buff[16] = {0};
  
  for (r=0; r<R; r++) {
    F(x_in + 16*(s[ r   %6]), B, c++, F_out_buff);    // x[s[(r+1)%6]] ^= F(x[s[ r   %6]],B,c++);
    for (int j = 0; j < 16; j++)
    {
      x_in[16*(s[(r+1)%6]) + j] ^= F_out_buff[j];
    }

    F(x_in + 16*(t[ r   %2]), B, c++, F_out_buff);    // x[s[(r+5)%6]] ^= F(x[t[ r   %2]],B,c++);
    for (int j = 0; j < 16; j++)
    {
      x_in[16*(s[(r+5)%6]) + j] ^= F_out_buff[j];
    }

    F(x_in + 16*(s[ (r+4)   %6]), B, c++, F_out_buff);    //  x[s[(r+3)%6]] ^= F(x[s[(r+4)%6]],B,c++);
    for (int j = 0; j < 16; j++)
    {
      x_in[16*(s[(r+3)%6]) + j] ^= F_out_buff[j];
    }
    
    F(x_in + 16*(s[ (r+2)   %6]), B, c++, F_out_buff);    //  x[t[(r+1)%2]] ^= F(x[s[(r+2)%6]],B,c++);
    for (int j = 0; j < 16; j++)
    {
      x_in[16*(s[(r+1)%6]) + j] ^= F_out_buff[j];
    }    
  }
  
}


void smallperm(uint8_t *x_in) {
  int R, r, c;
  
  if (B <= 3) {
    R = 6*B+3;
  } else {
    R = 6*B-9;
  }

  c=1;
  
  uint8_t F_out_buff[16] = {0};
  for (r=0; r<R; r++) {
    F(x_in + 16*(r%B), B, c++, F_out_buff);
    for (int j = 0; j < 16; j++)
    {
    x_in[16*(r+1)%B + j] ^= F_out_buff[j];           // x[(r+1)%B] ^= F(x[r%B],B,c++);
    }
    
    if (B == 4) {
      F(x_in + 16*((r+2)%B), B, c++, F_out_buff);
      for (int j = 0; j < 16; j++)
      {
        x_in[16*(r+3)%B + j] ^= F_out_buff[j];          // x[(r+3)%B] ^= F(x[(r+2)%B],B,c++);
      }
      
    }
  }
    
}


void doubleF(uint8_t *x_in, int r, int k) {
  uint8_t F_out_buff[16] = {0};
  if (r%2) {
    F(x_in + 16*(r+1), B, 2*k+1, F_out_buff);         // x[r  ] ^= F(x[r+1],B,2*k+1);
    for (int j = 0; j < 16; j++)
    {
    x_in[16*r + j] ^= F_out_buff[j];
    }

    F(x_in + 16*r, B, 2*k+2, F_out_buff);           // x[r+1] ^= F(x[r  ],B,2*k+2);
    for (int j = 0; j < 16; j++)
    {
    x_in[16*(r+1) + j] ^= F_out_buff[j];
    }

  } else {
    F(x_in + 16*r, B, 2*k+1, F_out_buff);           // x[r+1] ^= F(x[r  ],B,2*k+1);
    for (int j = 0; j < 16; j++)
    {
    x_in[16*(r+1) + j] ^= F_out_buff[j];
    }

    F(x_in + 16*(r+1), B, 2*k+2, F_out_buff);         // x[r  ] ^= F(x[r+1],B,2*k+2);
    for (int j = 0; j < 16; j++)
    {
    x_in[16*r + j] ^= F_out_buff[j];
    }
  }
}


void bigperm(uint8_t *x_in) {
  int j, r;
  int k = 0;
  int D = (B/2)*2;  
  
  for (j=0; j<3; j++) {
    if (D != B) {
      doubleF(x_in, B-2, k++);                  // doubleF(x, B-2, k++);
    }

    for (r=0; r<D-1; r++) {
      doubleF(x_in,r,k++);
      if (r != D-r-2) {
        doubleF(x_in, D-r-2, k++);
      }
    }

    if (D != B) {
      doubleF(x_in, B-2, k++);
    }
  }
}



