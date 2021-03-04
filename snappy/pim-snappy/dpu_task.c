#include <mram.h>
#include <defs.h>
#include <perfcounter.h>
#include <stdio.h>
#include "alloc.h"
#include "dpu_decompress.h"

// Comment out to count instructions
// #define COUNT_CYC

#define MAX_INPUT_SIZE (256 * 1024)
#define MAX_OUTPUT_SIZE (512 * 1024)

// WRAM variables
__host uint32_t req_idx[NR_TASKLETS];
__host uint32_t input_length[NR_TASKLETS];
__host uint32_t output_length[NR_TASKLETS];
__host uint32_t retval[NR_TASKLETS];
__host uint32_t perf;

// MRAM buffers
uint8_t __mram_noinit input_buffer[NR_TASKLETS][MAX_INPUT_SIZE];
uint8_t __mram_noinit output_buffer[NR_TASKLETS][MAX_OUTPUT_SIZE];

int main()
{
	struct in_buffer_context input;
	struct out_buffer_context output;
	uint8_t idx = me();
	if (idx == 0) {
		// Clear the heap, needed since we restart the program without
		// de-allocating and allocating the DPUs
		mem_reset();
	}

#ifdef COUNT_CYC
	perfcounter_config(COUNT_CYCLES, (idx == 0)? true : false);
#else
	perfcounter_config(COUNT_INSTRUCTIONS, (idx == 0)? true : false);
#endif

	printf("DPU starting, tasklet %d\n", idx);
	
	// Check that this tasklet has work to run 
	if (input_length[idx] == 0) {
		output_length[idx] = 0;
		printf("Tasklet %d has nothing to run\n", idx);
		return 0;
	}

	// Prepare the input and output descriptors
	input.cache = seqread_alloc();
	input.ptr = seqread_init(input.cache, input_buffer[idx], &input.sr);
	input.curr = 0;
	input.length = input_length[idx];

	output.buffer = output_buffer[idx];
	output.append_ptr = (uint8_t*)ALIGN(mem_alloc(OUT_BUFFER_LENGTH), 8);
	output.append_window = 0;
	output.read_buf = (uint8_t*)ALIGN(mem_alloc(OUT_BUFFER_LENGTH), 8);
	output.curr = 0;
	output.length = output_length[idx];

	// Do the uncompress
	if (dpu_uncompress(&input, &output))
	{
		printf("Tasklet %d: failed in %ld cycles\n", idx, perfcounter_get());
		retval[idx] = 0;
		return -1;
	}

#ifdef COUNT_CYC
	printf("Tasklet %d: %ld cycles, %d bytes\n", idx, perfcounter_get(), output.length);
#else
	printf("Tasklet %d: %ld instructions, %d bytes\n", idx, perfcounter_get(), output.length);
#endif	
	perf = perfcounter_get();
	retval[idx] = 1;
	return 0;
}

