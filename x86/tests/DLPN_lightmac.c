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

#include <lightmac/dlpn.h>

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

void print_memory_usage(){
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  printf( "Memory used %ld KB\n", usage.ru_maxrss);

}

int Deterministic_LPN_LightMAC(uint8_t x[T1], __m128i* KS){
  // Balanced diffuser = LightMAC with s=8 and AES4
  
  __m128i output = LightMAC_AES4_s8(x, KS);
  uint8_t flat_binary_output[128] = {0};

  // Unpacking the output into flat array of bits to apply UTRIBES
  for (int i = 0; i < 2; i++)
  {
    uint32_t tmp = (uint32_t) output[i] ;
    for (int j = 0; j < 32; j++)
    {
      flat_binary_output[32*i+j] = (tmp >> j)&1;
    }
  }
  
  return UTRIBES(flat_binary_output);
}
  
int main(){

  srand(time(NULL));
  printf("Start\n");

  __m128i user_key = _mm_set_epi32(0x12,0x65,0x2e,0x76);      // some random key
  __m128i Key_Schedule[10];

  AES_128_KEY_EXPANSION(Key_Schedule, user_key);
  
  printf("Key generated \n");
  // Random bytes = 16 bytes for each _m128i variable, which we have B * NUMBER_OF_SAMPLES many
  uint8_t* data = (uint8_t*)malloc(NUMBER_OF_SAMPLES * INPUT_BYTELENGTH * sizeof(uint8_t));
   if (data == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
      }
  size_t rng = getrandom(data, NUMBER_OF_SAMPLES * INPUT_BYTELENGTH, 0);
  if (rng==-1){
    printf("Error in generating random bytes");
  }
  
  
  // Warmup computation
  for (int sample_i = 0; sample_i < WARMUP_SAMPLES; sample_i++)
  {
    uint8_t sample[T1] = {0} ;  // input for lightMAC (of length T1) is LPN sample (of length VECTOR_LENGTH) in bytes concatenated with 10*
    for (int j =0; j < INPUT_BYTELENGTH; j+= 1)
    {
        sample[j] = data[INPUT_BYTELENGTH*sample_i + j];
    }
    sample[INPUT_BYTELENGTH] =  1;

    Deterministic_LPN_LightMAC(sample, Key_Schedule);
    
  }
  printf("Warmup done\n");
  
  
  // Actual benchmarking 
  
  uint64_t start_cycle, end_cycle;
  unsigned int dummy;
  struct timespec start_time, end_time;
  clock_gettime(CLOCK_REALTIME, &start_time);  
  printf("Start bench\n");
  start_cycle = _rdtscp(&dummy);    
  
  int count = 0;                                                                   // counter for noise

  for (int sample_i = 0; sample_i < NUMBER_OF_SAMPLES; sample_i++)
  {
    uint8_t sample[T1] = {0} ;  // input for lightMAC (of length T1) is LPN sample (of length VECTOR_LENGTH) in bytes concatenated with 10*
    for (int j =0; j < INPUT_BYTELENGTH; j+= 1)
    {
        sample[j] = data[INPUT_BYTELENGTH*sample_i + j];
    }
    sample[INPUT_BYTELENGTH] =  1;

    int noise = Deterministic_LPN_LightMAC(sample, Key_Schedule);
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








