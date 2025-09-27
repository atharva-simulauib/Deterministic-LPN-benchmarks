#ifndef DLPN_H
#define DLPN_H

#define VECTOR_LENGTH 2048
#define tau 5

#define INPUT_BYTELENGTH VECTOR_LENGTH/8
#define T2 INPUT_BYTELENGTH/15 + 1
#define T1 15*(INPUT_BYTELENGTH/15 + 1)

#if tau==33
    static int tribe_lengths[] = {2,4,5,6,11};
    static int total_tribes_length = 28;
#elif tau==10
    static int tribe_lengths[] = {4,5,7,10};
    static int total_tribes_length = 26;
#elif tau==5
    static int tribe_lengths[] = {5,6,9,10,11};
    static int total_tribes_length = 41;
#elif tau==1
    static int tribe_lengths[] = {7,9,12,17};
    static int total_tribes_length = 45;
#endif

__m128i AES128_ROUND_KEY_ASSIST(__m128i tmp1, __m128i tmp2);

void AES_128_KEY_EXPANSION(__m128i key_schedule[10], __m128i k);

__m128i AES_FOUR_ROUNDS(__m128i x, __m128i* KS);

__m128i LightMAC_AES4_s8(uint8_t input[], __m128i KS[10]);

#endif // DLPN_H