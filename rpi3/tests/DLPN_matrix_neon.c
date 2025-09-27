#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arm_neon.h>
#include <time.h>
#include <sys/random.h>
#include <sys/resource.h>

#include "libpopcnt.h"
#include <matrix/dlpn.h>

// includes for perf_event_open https://learn.arm.com/learning-paths/servers-and-cloud-computing/arm_pmu/perf_event_open/
#include <linux/perf_event.h> /* Definition of PERF_* constants */
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>
#include <inttypes.h>

#define NUMBER_OF_SAMPLES 10000
#define WARMUP_SAMPLES (NUMBER_OF_SAMPLES / 4)
#define inlen (VECTOR_LENGTH / 8)     // in bytes = VECTOR_LENGTH / 8
#define size_popcount 16 //for libpopcount
#define MATRIX_ROWSIZE              // 128 (matrix can be VECTOR_LENGTH VECTOR_LENGTH x VECTOR_LENGTH or 128 x VECTOR_LENGTH)
uint8_t fixed_matrix[MATRIX_ROWSIZE][VECTOR_LENGTH / 8];  // fixed matrix (diffuser)

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

// Function for matrix-vector multiplication using NEON intrinsics
void matrix_vector_multiply_neon(uint8_t fixed_matrix[][VECTOR_LENGTH / 8],
                                 const uint8_t* vector_ptr,
                                 uint8_t* result,
                                 int vector_size_bytes, int matrix_number_of_rows){

    // Process each row of the matrix
    for (int row = 0; row < (matrix_number_of_rows); row++) {
        uint64_t dot_product = 0;

        // Process the row in chunks of 16 bytes (128 bits)
        for (int j = 0; j < vector_size_bytes; j += 16) {
            // Load 16 bytes from the matrix row and vector
            uint8x16_t matrix_neon = vld1q_u8(&fixed_matrix[row][j]);
            uint8x16_t vector_neon = vld1q_u8(&vector_ptr[j]);

            // Perform bitwise AND
            uint8x16_t mult_neon = vandq_u8(matrix_neon, vector_neon);

            // Count set bits (population count)
            //uint8x16_t popcnt_neon = vcntq_u8(mult_neon); // Count set bits in each byte
            //uint64_t popcnt = vaddvq_u64(vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(popcnt_neon)))); // Horizontally sum
            uint64_t popcount = popcnt(&mult_neon, size_popcount);
            dot_product +=popcount;  // Ensure the result fits in 8 bits
        }

        // If the dot product is odd, set the corresponding bit in the result
        if (dot_product % 2 == 1) {
            result[row / 8] |= (1 << (row % 8));
        }
    }
}

int main() {
    srand(time(NULL));
    int noise_count = 0;

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

    // Generate a random fixed matrix
    for (int i = 0; i < MATRIX_ROWSIZE; i++) {
        for (int j = 0; j < VECTOR_LENGTH / 8; j++) {
            fixed_matrix[i][j] = (uint8_t)(rand() % 256); // Generate random byte (0-255)
        }
    }

    // warmup computation
    for (int sample_index = 0; sample_index < WARMUP_SAMPLES; sample_index++) {
        uint8_t* sample_ptr = data + sample_index * (VECTOR_LENGTH / 8);
        uint8_t result[MATRIX_ROWSIZE / 8] = {0};
        matrix_vector_multiply_neon(fixed_matrix, sample_ptr, result, VECTOR_LENGTH / 8, MATRIX_ROWSIZE);
        // UTRIBES on output
        uint8_t flat_binary_output[128] = {0};
        // Unpacking the output into flat array of bits to apply UTRIBES
        for (int i = 0; i < 16; i++){
            for (int j = 0; j < 8; j++)
            {
                flat_binary_output[8*i+j] = (output[i] >> j)&1;
            }
        }

        int noise = UTRIBES(flat_binary_output);
    }
    
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

    // Perform matrix-vector multiplication for each sample
    for (int sample_index = 0; sample_index < NUMBER_OF_SAMPLES; sample_index++) {
        uint8_t* sample_ptr = data + sample_index * (VECTOR_LENGTH / 8);
        uint8_t result[MATRIX_ROWSIZE / 8] = {0};
        matrix_vector_multiply_neon(fixed_matrix, sample_ptr, result, VECTOR_LENGTH / 8, MATRIX_ROWSIZE);
        // UTRIBES on output
        uint8_t flat_binary_output[128] = {0};
        // Unpacking the output into flat array of bits to apply UTRIBES
        for (int i = 0; i < 16; i++){
            for (int j = 0; j < 8; j++)
            {
                flat_binary_output[8*i+j] = (output[i] >> j)&1;
            }
        }

        int noise = UTRIBES(flat_binary_output);
        if (noise==1)
        {
            noise_count +=1:
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

// gcc tests/DLPN_matrix_neon.c -Icommon/matrix -mfpu=neon -O3 -o test && ./test
