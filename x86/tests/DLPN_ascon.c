#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <immintrin.h>
#include <x86intrin.h>
#include <time.h>
#include <sys/random.h>
#include <sys/resource.h>

#define NUMBER_OF_SAMPLES 100000
#define WARMUP_SAMPLES NUMBER_OF_SAMPLES/4

#include <asconprfref/dlpn.h>

#define inlen VECTOR_LENGTH/8     // in bytes = VECTOR_LENGTH/8
#define outlen 32    

int UTRIBES(uint8_t x[]){
    int number_of_tribes = sizeof(tribe_lengths) / sizeof(tribe_lengths[0]);
    
    int curr_length = 0; int curr_index = 0;
    int res = 0; int AND = 1;
    
    for (int i = 0; i < total_tribes_length; i++)
    {
        AND = (AND & x[i]);
        if (i == curr_length + tribe_lengths[curr_index]-1)
        {
            res = ( res | AND );
            curr_length += tribe_lengths[curr_index];
            curr_index += 1;
            AND = 1;
        }
    }
    return res;    
}

int Deterministic_LPN_Ascon(unsigned char* output, unsigned char* sample, unsigned char* s){
  // Balanced diffuser = Ascon-PRF
  crypto_prf(output, outlen, sample, inlen, s);

  // Unpacking the output into flat array of bits to apply UTRIBES
  uint8_t binary_output[128] ;
  
  for (int i = 0; i < 16; i++){
    uint8_t tmp = output[i];
    for (int j = 0; j < 8; j++){
      binary_output[8*i+j] = (tmp >> j)&1 ;
    }
  }
  
  return UTRIBES(binary_output);
          
}


void print_memory_usage(){
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  printf( "Memory used %ld MB\n", usage.ru_maxrss/1000);

}


int main(){
  srand(time(NULL));
  int count = 0;

  unsigned char s[inlen];                     // LPN Secret 
  size_t rng = getrandom(s, inlen,0);
  if (rng==-1){
    printf("Error in generating random bytes");
  }

  // Random bytes = inlen * NUMBER_OF_SAMPLES many
  uint8_t* data = (uint8_t*)malloc(NUMBER_OF_SAMPLES * B * 16 * sizeof(uint8_t));

  if (data == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
      }

  rng = getrandom(data, NUMBER_OF_SAMPLES * B * 16,0);
  if (rng==-1){
    printf("Error in generating random bytes");
  }
  

  // Warmup computation
  for (int sample_i = 0; sample_i < WARMUP_SAMPLES; sample_i++)
  {
    unsigned char sample[inlen];             // LPN sample
    unsigned char output[outlen];
    
    for (int j = 0; j < inlen; j++){
      sample[j] = data[sample_i * inlen + j];
    }
    Deterministic_LPN_Ascon(output, sample, s);
  }
  
  printf("Warmup done\n");

  // Actual benchmarking 
  
  uint64_t start_cycle, end_cycle;
  unsigned int dummy;
  struct timespec start_time, end_time;
  clock_gettime(CLOCK_REALTIME, &start_time);  
  printf("Start bench\n");
  start_cycle = _rdtscp(&dummy);   
  for (int sample_i = 0; sample_i < NUMBER_OF_SAMPLES; sample_i++)
  {
    unsigned char sample[inlen];             // LPN sample 
    unsigned char output[outlen];
    
    for (int j = 0; j < inlen; j++){
      sample[j] = data[sample_i * inlen + j];
    }
    int noise = Deterministic_LPN_Ascon(output, sample, s);    
    if (noise==1)
    {
      count+=1;
    }
    
  }

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