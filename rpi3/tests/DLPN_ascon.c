#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/random.h>
#include <sys/resource.h>

#define NUMBER_OF_SAMPLES 10000
#define WARMUP_SAMPLES NUMBER_OF_SAMPLES/4

#include <dlpn.h>

#define inlen VECTOR_LENGTH/8     // in bytes = VECTOR_LENGTH/8
#define outlen 32    

// includes for perf_event_open https://learn.arm.com/learning-paths/servers-and-cloud-computing/arm_pmu/perf_event_open/
#include <linux/perf_event.h> /* Definition of PERF_* constants */
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>
#include <inttypes.h>


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


int main(){
  srand(time(NULL));
  int noise_count = 0;                              // count noise bits = 1

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
      noise_count+=1;
    } 
  }
  //------------------------------------------------------------------------------------
  // Stop counting
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

  // Read and print result
  read(fd, &val, sizeof(val));
  printf("Instructions retired: %"PRIu64"\n", val);

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
  return 0;
}



// gcc tests/DLPN_ascon.c -Icommon/asconprfv3armv6/  common/asconprfv3armv6/*.c -mfpu=neon -O3 -o test && ./test
