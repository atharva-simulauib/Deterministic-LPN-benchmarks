#ifndef DLPN_H
#define DLPN_H

#define VECTOR_LENGTH  4096
#define tau 1

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



#endif // DLPN_H
