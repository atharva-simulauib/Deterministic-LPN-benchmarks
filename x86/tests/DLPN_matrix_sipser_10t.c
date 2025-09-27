#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <m4ri/mzd.h>
#include <m4ri/m4ri.h>
#include <m4ri/config.h>
#include <m4ri/misc.h>
#include <m4ri/djb.h>
#include <x86intrin.h>
#include <time.h>
#include <sys/random.h>
#include <sys/resource.h>


#define VECTOR_LENGTH 512

#if VECTOR_LENGTH==512
    #define LAMBDA 16
#elif VECTOR_LENGTH==1024
    #define LAMBDA 22
#elif VECTOR_LENGTH==2048
    #define LAMBDA 32
#endif
#define W 2

#define NUMBER_OF_SAMPLES 100000
#define clear 1


int SIPSER(uint8_t x[]){
    int res = 0;
    for (int i = 0; i < LAMBDA; i++)
    {
        int AND = 1;
        for (int j = i*LAMBDA*W; j < (i+1)*LAMBDA*W; j+=W){
            int OR = 0;
            for (int k = j; k < j+W; k++)
            {
                OR = (OR | x[k]) ;
            }
            
            AND = AND & OR;
        }
        res = ( res | AND );
    }

    return res;
}

void print_memory_usage(){
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  printf( "Memory used %ld KB\n", usage.ru_maxrss);

}


int main(){

    mzd_t* A;                                // predefined matrix for multiplication 
    A = mzd_init(VECTOR_LENGTH, VECTOR_LENGTH);
    mzd_randomize(A);                        // randomize the matrix
    //djb_t *djb_A = djb_compile(A);         //djb pre-computation
    int count = 0;

    // Random bytes = 16 bytes for each sample NUMBER_OF_SAMPLES many
    uint8_t* data = (uint8_t*)malloc(NUMBER_OF_SAMPLES * (VECTOR_LENGTH/8) * sizeof(uint8_t));
    if (data == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
        }
    size_t rng = getrandom(data, NUMBER_OF_SAMPLES * (VECTOR_LENGTH/8),0);
    if (rng==-1)
    {
        printf("Error in generating random bytes");
    }


    // Warmup computation 
    for (int i = 0; i < NUMBER_OF_SAMPLES/4; i++)
    {
        mzd_t *v = mzd_init(1,VECTOR_LENGTH);                       // LPN sample vector
        uint64_t *x = (uint64_t*) malloc( 16 * sizeof(uint64_t)); 
        if (x == NULL) {
            printf("Memory allocation failed for uint64_t array\n");
            free(v);
            return 1;
            }
        
        for (int j = 0; j < 16; j++)
        {
            x[j] = data[16*i+j];                                   

        v->data = x;                                                // fill random bytes in a string and point the mzd_t data type to it
        }

        mzd_t *C = mzd_init(1,VECTOR_LENGTH);                       // matrix for storing multiplication result      v = 1 x VECTOR_LENGTH times A = VECTOR_LENGTH x VECTOR_LENGTH 
        //djb_apply_mzd(djb_A, C, v); 
        mzd_t *D = _mzd_mul_va(C, v, A, clear);                     // v*A multiplication function from mzd.c
        //mzd_t *C = _mzd_mul_va(    NULL, A, v, 0);
        
        // Unpacking output
        uint64_t* output = (uint64_t*) C->data;
        uint8_t binary_output[VECTOR_LENGTH];
        for (int i1 = 0; i1 < VECTOR_LENGTH/64; i1++)
        {
            uint64_t tmp = output[i1];  
            for (int j1 = 0; j1 < 64; j1++)
            {
                binary_output[64*i1 + j1] = (tmp >> j1)&1;
            }
        }

        int noise = SIPSER(binary_output);                           // noise bit = SIPSER(A*v)
        
        
    }
    printf("Warmup done\n");

    

    // Actual benchmark
    uint64_t start_cycle, end_cycle;
    unsigned int dummy;
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time);  
    printf("Start bench\n");
    start_cycle = _rdtscp(&dummy);     

    for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
    {
        mzd_t *v = mzd_init(1,VECTOR_LENGTH);                      // LPN sample vector

        uint64_t *x = (uint64_t*) malloc( 16 * sizeof(uint64_t));   // fill random bytes in a string and point the mzd_t data type v to it
        if (x == NULL) {
            printf("Memory allocation failed for uint64_t array\n");
            free(v);
            return 1;
            }
        
        for (int j = 0; j < 16; j++)
        {
            x[j] = data[16*i+j];
        }

        v->data = x;                                         
        mzd_t *C = mzd_init(1,VECTOR_LENGTH);                   // matrix for storing multiplication result      v = 1 x VECTOR_LENGTH times A = VECTOR_LENGTH x VECTOR_LENGTH 
        //djb_apply_mzd(djb_A, C, v); 
        mzd_t *D = _mzd_mul_va(C, v, A, clear);               // v*A multiplication function from mzd.c
        //mzd_t *C = _mzd_mul_va(    NULL, A, v, 0); += 1;
        
        // unpacking output
        uint64_t* output = (uint64_t*) C->data;
        uint8_t binary_output[VECTOR_LENGTH];
        for (int i1 = 0; i1 < VECTOR_LENGTH/64; i1++)
        {
            uint64_t tmp = output[2*i1];  
            for (int j1 = 0; j1 < 64; j1++)
            {
                binary_output[64*i1 + j1] = (tmp >> j1)&1;
            }
            
        }
        
        int noise = SIPSER(binary_output);                           // noise bit = SIPSER(A*v)
        if (noise==1)
        {
            count += 1;
        }
        
        
        
    }
    
    
    mzd_free(A);
    
    
    printf("Multiplication done\n");
    end_cycle = _rdtscp(&dummy);
    clock_gettime(CLOCK_REALTIME, &end_time);
    
    double diff_cycles = (double) (end_cycle-start_cycle)/ NUMBER_OF_SAMPLES;
    double noise_rate = (double) count / NUMBER_OF_SAMPLES;
    
    // Printing results  
    printf("\nTotal Number of samples: %d Vector lengh: %d\n", NUMBER_OF_SAMPLES, VECTOR_LENGTH);
    printf("Number of samples for which noise bit is 1: %d\n", count);
    printf("Noise rate: %f \n\n", noise_rate );
    
    double total_time_s = (double) (end_time.tv_sec - start_time.tv_sec) ;
    double total_time_ns = (double) (end_time.tv_nsec - start_time.tv_nsec) ;
    double total_time_ms = total_time_s*1000 + total_time_ns/1000000;
    
    printf("Average number of CPU cycles: %f\n", diff_cycles);
    printf("Time in milliseconds: %f ms\n", total_time_ms);
    print_memory_usage();
    
    return 0;

}