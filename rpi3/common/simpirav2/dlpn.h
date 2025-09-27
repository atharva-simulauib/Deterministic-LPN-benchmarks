#ifndef DLPN_H
#define DLPN_H

#define VECTOR_LENGTH 512
#define tau 1
#define B VECTOR_LENGTH/128
#define INPUT_BYTELENGTH VECTOR_LENGTH/8

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


void F(const uint8_t *x_in, int b, int c, uint8_t *x_out);
void perm8(uint8_t *x_in);
void smallperm(uint8_t *x_in);
void bigperm(uint8_t *x_in);

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
