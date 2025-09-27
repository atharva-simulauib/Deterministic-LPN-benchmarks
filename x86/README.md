# Deterministic-LPN benchmarks on x86
Below we specify how to run tests for each Diffuse. The programs are compiled with `gcc` and we use certain POSIX specific libraries (`sys/random.h` for randomness) so user can use WSL, for example if on Windows.

### **Simpira**

- Code obtained from https://mouha.be/simpira/ and the downloaded C files are in `/common/simpira`. We used the file Permutations_Ref.c which contain reference implementation for all simpira permutations indexed by `B` where `B` is equal to VECTOR_LENGTH/128
- We create a header file dlpn.h in /common/simpira to include the necessary permutation and this is where we define the macros VECTOR_LENGTH and tau. 
- Change the values of macros `VECTOR_LENGTH` and `tau` in `/common/simpira/dlpn.h` to run tests for different parameters.

To perform test, run:
```bash
gcc tests/DLPN_simpira.c  -Icommon common/simpira/Permutations_Ref.c -march=native -O2 -maes -o test && ./test
```


### **AsconPRF**

- Code obtained from https://github.com/ascon/ascon-c/. The directory is cloned and followed the build instructions. Then use the reference implementation of Ascon PRF from `crypto_auth/asconprfv13/ref` and place it in `common/asconprfref`
- We create a header file dlpn.h in common/asconprfref to include the necessary prf and this is where we define the macros VECTOR_LENGTH and tau. 
- Change the values of macros `VECTOR_LENGTH` and `tau` in `/common/asconprfref/dlpn.h` to run tests for different parameters.

To perform benchmark, run:
```bash
gcc tests/DLPN_ascon.c -Icommon common/asconprfref/*.c -march=native -O2 -o test.exe && ./test.exe
```


### **LightMAC**

- We implemented LightMAC with $s=8$. Implementation can be found in `common/lightmac/lightmacAES4.c`. This implementation makes use of AES intrinsics.
- We create a header file dlpn.h in common/lightmac to include the necessary MAC and this is where we define the macros VECTOR_LENGTH and tau. 
- Change the values of macros `VECTOR_LENGTH` and `tau` in `common/LightMAC/dlpn.h` to run tests for different parameters.

To perform benchmark, run:
```bash
gcc tests/DLPN_lightmac.c -Icommon common/LightMAC/lightmacAES4.c -march=native -O2 -maes -o test.exe && ./test.exe
```


### **Matrix**

- First need to set up m4ri https://bitbucket.org/malb/m4ri and place it in the same directory. 
- The file `tests/DLPN_matrix.c` contains the implementation of DLPN instantiated with matrix multiplication as Diffuse.
- Change the values of macros `VECTOR_LENGTH` and `tau` in `tests/DLPN_matrix.c` to run tests for different parameters.

To perform benchmark, run:
```bash
gcc tests/DLPN_matrix.c -Im4ri m4ri/m4ri/mzd.c m4ri/m4ri/misc.c m4ri/m4ri/mmc.c m4ri/m4ri/graycode.c -march=native -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -msha -maes -mavx -mfma -mavx2  -g -O2 -o test.exe && ./test.exe
```


### **Matrix with SIPSER**

- We implementation the SIPSER function parmatrized by `LAMBDA` in `tests/DLPN_matrix_sipser.c`

To perform benchmark, run:
```bash
gcc tests/DLPN_matrix_sipser.c -Im4ri m4ri/m4ri/mzd.c m4ri/m4ri/misc.c m4ri/m4ri/mmc.c m4ri/m4ri/graycode.c -march=native -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -msha -maes -mavx -mfma -mavx2  -g -O2 -o test.exe  && ./test.exe
```
