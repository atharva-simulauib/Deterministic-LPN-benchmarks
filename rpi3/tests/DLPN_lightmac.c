#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/random.h>
#include <sys/resource.h>

#include <arm_neon.h>

#include "tinyAES/aes.h"
#include "dlpn.h"

// includes for perf_event_open https://learn.arm.com/learning-paths/servers-and-cloud-computing/arm_pmu/perf_event_open/
#include <linux/perf_event.h> /* Definition of PERF_* constants */
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>
#include <inttypes.h>

#define NUMBER_OF_SAMPLES 10000
#define WARMUP_SAMPLES NUMBER_OF_SAMPLES/4

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


int Deterministic_LPN_LightMAC(const struct AES_ctx* ctx, uint8_t input[T1], uint8_t* output){
  /* LightMAC construction using AES_FOUR_ROUNDS as the block cipher 
  For a block cipher operating on input of length n-bits we choose a parameter s for LightMAC 
  and the input for block cipher is (n-s) bits of given input concatenated with i_s which is s-bit representation of i
  Here we have s = 8. So we parse the input as bytes. Block cipher input is 15 bytes of given input concatenated with 1 byte of i_s 
  __m128i* Key_Schedule;*/
  uint8x16_t res = vdupq_n_u8(0);
  for (uint8_t i = 0; i < T2; i++){
    // input for AES 4 will be byte representation of i concatenated with 15 bytes from LPN input
    uint8_t tmpData[16];
    tmpData[15] = i;                 // counter i as the last byte
    for (int j = 0; j < 15; j++)
    {
      tmpData[j] = input[ 15*i + j];
    }
    
    AES4_ECB_encrypt(ctx, tmpData);
    uint8x16_t tmp = vld1q_u8(tmpData);
    res = veorq_u8(res, tmp);
  }  
  vst1q_u8(output, res);
  AES4_ECB_encrypt_last(ctx, output);
  
  // UTRIBES on output
  uint8_t flat_binary_output[128] = {0};

  // Unpacking the output into flat array of bits to apply UTRIBES
  for (int i = 0; i < 16; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      flat_binary_output[8*i+j] = (output[i] >> j)&1;
    }
  }
  
  return UTRIBES(flat_binary_output);
}
 
 
void print_memory_usage(){
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  printf( "Memory used %ld MB\n", usage.ru_maxrss/1000);

}
 
// Executes perf_event_open syscall and makes sure it is successful or exit
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags){
  int fd;
  fd = syscall(SYS_perf_event_open, hw_event, pid, cpu, group_fd, flags);
  if (fd == -1) {
    fprintf(stderr, "Error creating event");
    exit(EXIT_FAILURE);
  }

  return fd;
}



int main() {
    srand(time(NULL));
    int noise_count = 0;                              // count noise bits = 1


    // Allocate memory for random input data
    uint8_t* data = (uint8_t*)malloc(NUMBER_OF_SAMPLES * (VECTOR_LENGTH / 8) * sizeof(uint8_t));
    if (data == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // Generate random data
    ssize_t rng = getrandom(data, NUMBER_OF_SAMPLES * (VECTOR_LENGTH / 8), 0);
    if (rng == -1) {
        printf("Error in generating random bytes\n");
        free(data);
        return 1;
    }

    // Initialize AES
    uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    
    printf("\nAES ctx initialized\n");

     
    // Warmup computation
    for (int sample_i = 0; sample_i < WARMUP_SAMPLES; sample_i++)
    {
      uint8_t sample[T1] = {0} ;  // input for lightMAC (of length T1) is LPN sample (of length VECTOR_LENGTH) in bytes concatenated with 10*
      for (int j =0; j < INPUT_BYTELENGTH; j+= 1)
      {
          sample[j] = data[INPUT_BYTELENGTH*sample_i + j];
      }
      sample[INPUT_BYTELENGTH] =  1;
      uint8_t result[16] = {0};
      Deterministic_LPN_LightMAC(&ctx, sample, result);
    }
    printf("Warmup done\n");
        

    
    // Actual benchmarking 
    //------------------------------------------------------------------------------------
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time);  
    printf("Start bench\n");
    // use perf_event_open
    int fd;
    uint64_t  val;
    struct perf_event_attr  pe;

    // Configure the event to count
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    pe.exclude_kernel = 1;   // Do not measure instructions executed in the kernel
    pe.exclude_hv = 1;  // Do not measure instructions executed in a hypervisor

    // Create the event
    fd = perf_event_open(&pe, 0, -1, -1, 0);

    //Reset counters and start counting
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    //-----------------------------------------------------------------------------------

    // Compute LightMAC for each sample
    for (int sample_i = 0; sample_i < NUMBER_OF_SAMPLES; sample_i++)
    {
      uint8_t sample[T1] = {0} ;  // input for lightMAC (of length T1) is LPN sample (of length VECTOR_LENGTH) in bytes concatenated with 10*
      for (int j =0; j < INPUT_BYTELENGTH; j+= 1)
      {
          sample[j] = data[INPUT_BYTELENGTH*sample_i + j];
      }
      sample[INPUT_BYTELENGTH] =  1;
      uint8_t result[16] = {0};
      int noise = Deterministic_LPN_LightMAC(&ctx, sample, result);
      if (noise==1){
      noise_count+=1;
      } 
    }
    //------------------------------------------------------------------------------------
    // Stop counting
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

    // Read and print result
    read(fd, &val, sizeof(val));
    printf("Instructions retired: %"PRIu64"\n", val);
    // End performance counter
    clock_gettime(CLOCK_REALTIME, &end_time);

    double diff_cycles = (double) (val)/ NUMBER_OF_SAMPLES;
    double noise_rate = (double) noise_count / NUMBER_OF_SAMPLES;

    // Printing results  
    printf("\nTotal Number of samples: %d Vector lengh: %d\n", NUMBER_OF_SAMPLES, VECTOR_LENGTH);
    printf("Number of samples for which noise bit is 1: %d\n", noise_count);
    printf("Noise rate: %f \n\n", noise_rate );

    double total_time_s = (double) (end_time.tv_sec - start_time.tv_sec) ;
    double total_time_ns = (double) (end_time.tv_nsec - start_time.tv_nsec) ;
    double total_time_ms = total_time_s*1000 + total_time_ns/1000000;
    
    printf("Average number of CPU cycles: %f\n", diff_cycles);
    printf("Time in milliseconds: %f ms\n", total_time_ms);
    print_memory_usage();
    // Clean up file descriptor
    close(fd);
    // Free allocated memory
    free(data);

    return 0;
}



// gcc tests/DLPN_lightmac.c -Icommon/lightmac common/lightmac/tinyAES/aes.c -mfpu=neon -O3 -o test && test
