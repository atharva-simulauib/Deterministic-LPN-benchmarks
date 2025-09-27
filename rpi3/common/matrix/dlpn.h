#ifndef DLPN_H
#define DLPN_H

#define VECTOR_LENGTH 1024  
#define tau 33                        // tau/100

#define B VECTOR_LENGTH/128          // Number of 128-bit words

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
