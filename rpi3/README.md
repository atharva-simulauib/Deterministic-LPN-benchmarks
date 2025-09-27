# Deterministic-LPN benchmarks on Raspberry Pi 3

We ran the tests on Raspberry Pi running 32 bit RPi OS. Below we specify how to run tests for each Diffuse. The programs are compiled with `gcc`.

### **Simpira**

- We use [TinyAES](https://github.com/kokke/tiny-AES-c) which is a pure C implementation of AES, and define relevant function for 1 round. These are used in`common/simpirav2/simpira_ref.c` which contain reference implementation for all simpira permutations indexed by `B` where `B` is equal to VECTOR_LENGTH/128
- We create a header file dlpn.h in /common/simpira to include the necessary permutation and this is where we define the macros VECTOR_LENGTH and tau. 
- Change the values of macros `VECTOR_LENGTH` and `tau` in `/common/simpira/dlpn.h` to run tests for different parameters.

To perform benchmark, run:
```bash
gcc tests/DLPN_simpira.c -Icommon/simpirav2 common/simpirav2/tinyAES/aes.c -mfpu=neon -O3 -o test.exe && ./test.exe
```


### **Ascon**

- Code obtained from https://github.com/ascon/ascon-c/. and we use the optimzed implementation for ARMv6 (which is also supported for ARMV7la) from `crypto_auth/asconprfv13/armv6` and place it in `common/asconprfv3armv6`
- We create a header file dlpn.h in common/asconprfref to include the necessary prf and this is where we define the macros VECTOR_LENGTH and tau. 
- Change the values of macros `VECTOR_LENGTH` and `tau` in `/common/asconprfref/dlpn.h` to run tests for different parameters.

To perform benchmark, run:
```bash
gcc tests/DLPN_ascon.c -Icommon/asconprfv3armv6/  common/asconprfv3armv6/*.c -mfpu=neon -O3 -o test.exe && ./test.exe
```


### **LightMAC**

- We use TinyAES implemented LightMAC with $s=8$. Implementation can be found in `common/lightmac/lightmacAES4.c`
- We create a header file dlpn.h in common/lightmac to include the necessary MAC and this is where we define the macros VECTOR_LENGTH and tau. 
- Change the values of macros `VECTOR_LENGTH` and `tau` in `common/lightmac/lightmacAES4.c` to run tests for different parameters.

To perform benchmark, run:
```bash
gcc tests/DLPN_lightmac.c -Icommon/lightmac common/lightmac/tinyAES/aes.c -mfpu=neon -O3 -o test.exe && ./test.exe

```


### **Matrix**

- We wrote our own implementation utilising ARM NEON intrinsic for bitwise AND, and using `libpopcount.h` for population count of bits, meaning XOR of bits.
- The file `tests/DLPN_matrix_neon.c` contains the implementation of DLPN instantiated with matrix multiplication as Diffuse.
- Change the values of macros `VECTOR_LENGTH` and `tau` in `common/matrix/dlpn.h` to run tests for different parameters.

To perform benchmark, run:
```bash
gcc tests/DLPN_matrix_neon.c -Icommon/matrix -mfpu=neon -O3 -o test.exe && && ./test.exe
```
