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

#include <simpira/dlpn.h>

__m128i random_m128(){
    // Generates random m128 type vector
    uint8_t tmp[16];
    for (int i = 0; i < 16; i++)
    {
        tmp[i] = rand()%256;            // generate random bytes
    }
    
    // Load the 16 8-bit values into the __m128i type variable x
    __m128i x = _mm_set_epi8(tmp[15], tmp[14], tmp[13], tmp[12], tmp[11], tmp[10], tmp[9], tmp[8],
                        tmp[7], tmp[6], tmp[5], tmp[4], tmp[3], tmp[2], tmp[1], tmp[0]);
    return x;
}



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
  printf( "Memory used %ld MB\n", usage.ru_maxrss/1000);

}


int Deterministic_LPN_Simpira(__m128i x[B]){
  // Balanced diffuser = Simpirav2
  simpira_perm(x);
  uint8_t binary_output[128] = {0};

  // Unpacking the output into flat array of bits to apply UTRIBES
  for (int i = 0; i < 2; i++)
  {
    uint32_t tmp = x[0][i];                   
    for (int j = 0; j < 32; j++){
      binary_output[32*i+j] = (tmp >> j)&1 ;
    }
  }
  return UTRIBES(binary_output);
          
}

int main(){
  srand(time(NULL));
  printf("Start\n");

  __m128i s[B];                                                       // LPN Secret 
  for (int i = 0; i < B; i++)
  {
    s[i] = random_m128();
  }
  printf("Secret generated\n");


  // Random bytes = 16 bytes for each of the B _m128i variable, for every sample 
  uint8_t* data = (uint8_t*)malloc(NUMBER_OF_SAMPLES * B * 16 * sizeof(uint8_t));
  if (data == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
      }

  size_t rng = getrandom(data, NUMBER_OF_SAMPLES * B * 16,0);
  if (rng==-1)
  {
    printf("Error in generating random bytes");
  }
  

  // Warmup computation
  for (int sample_i = 0; sample_i < WARMUP_SAMPLES; sample_i++)
  {
    __m128i sample[B];                                                              // LPN sample
    for (int word_i = 0; word_i < B; word_i+= 16)
    {
      sample[word_i] = _mm_loadu_si128((__m128i*)&data[sample_i + word_i]);         // load random bytes
    }
    int noise = Deterministic_LPN_Simpira(sample);
  }
  printf("Warmup finished\n");

  
  // Actual benchmarking 
  
  uint64_t start_cycle, end_cycle;
  unsigned int dummy;
  struct timespec start_time, end_time;
  clock_gettime(CLOCK_REALTIME, &start_time);  
  printf("Start bench\n");
  start_cycle = _rdtscp(&dummy);    
  
  int count = 0;                                                                   // Counter for noise bit 

  for (int sample_i = 0; sample_i < NUMBER_OF_SAMPLES; sample_i++)
  {
    __m128i sample[B];                                                             // LPN sample 
    for (int word_i = 0; word_i < B; word_i+= 16)
    {
      sample[word_i] = _mm_loadu_si128((__m128i*)&data[sample_i + word_i]);
      sample[word_i] = _mm_xor_si128(sample[word_i], s[word_i]);                   // Mixing LPN secret
    }
    int noise = Deterministic_LPN_Simpira(sample);                                 // GenTRIBES( Simpira(s + x_i) )
    if (noise == 1)
    {
      count += 1;
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
