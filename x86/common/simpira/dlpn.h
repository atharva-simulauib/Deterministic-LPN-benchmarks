#ifndef DLPN_H
#define DLPN_H

#define VECTOR_LENGTH 1024
#define tau 33

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

#define B VECTOR_LENGTH/128

__m128i F(__m128i x, int b, int c) ;

__m128i F_last(__m128i x, int b, int c) ;

__m128i Fi(__m128i x, int b, int c) ;

__m128i Fi_last(__m128i x, int b, int c) ;

void perm8(__m128i x[B]);
void smallperm(__m128i x[B]);
void bigperm(__m128i x[B]);

#if B==4
    #define simpira_perm smallperm
#elif B==8
    #define simpira_perm perm8
#elif B==16
    #define simpira_perm bigperm
#elif B==32
    #define simpira_perm bigperm
#endif



#endif // DLPN_H