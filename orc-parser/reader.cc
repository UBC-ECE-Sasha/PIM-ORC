#include "orc/OrcFile.hh"
#include "orc/ColumnPrinter.hh"
#include "orc/Statistics.hh"
#include "pim-snappy/pim_snappy.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y)) 

using namespace orc;

struct thread_args {
	int thread_num;
	char* filename;
	uint64_t row_number;
	uint64_t sum;
};

/**
 * Stripe reader thread.
 *
 * @param arg: pointer to thread_args struct
 */
void *read_thread(void *arg) {
	struct thread_args *args = (struct thread_args *)arg;

	// Read in the file as a stream
	ORC_UNIQUE_PTR<InputStream> inStream = readLocalFile(args->filename);

	// Allocate the ORC reader
	ReaderOptions readerOpts;
	ORC_UNIQUE_PTR<Reader> reader = createReader(std::move(inStream), readerOpts);

	// Allocate the row reader	
	RowReaderOptions rowReaderOptions;
	ORC_UNIQUE_PTR<RowReader> rowReader = reader->createRowReader(rowReaderOptions);
	ORC_UNIQUE_PTR<ColumnVectorBatch> batch = rowReader->createRowBatch(reader->getRowIndexStride());

	// Seek to this thread's row
	rowReader->seekToRow(args->row_number);

	StructVectorBatch *root = dynamic_cast<StructVectorBatch *>(batch.get());
	LongVectorBatch *first_col = dynamic_cast<LongVectorBatch *>(root->fields[0]); // Get first column

	// read the row

	for (uint64_t row = 0; row < 1; row++) {
		if (!rowReader->next(*batch))
			break;
		
		for (uint64_t elem = 0; elem < batch->numElements; elem++) {
			if (first_col->notNull[elem]) {
				args->sum += first_col->data[elem]; }
		}
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	int opt;
	char *input_file = NULL;
	uint64_t num_threads = 1;

	while ((opt = getopt(argc, argv, "f:t:")) != -1) {
		switch(opt) {
			case 'f':
				input_file = optarg;
				break;
			case 't':
				num_threads = atoi(optarg);
				break;
			default:
				std::cout << "Unknown Option: " << optopt << "\n";
				exit(1);
		}
	}

	if (input_file == NULL) {
		std::cout << "Specify an input file with -f\n";
		exit(1);
	}

	// Start the DPU thread
#if USE_PIM
	pim_init();
#endif

	// Do some initial processing of the file to find where to break it up
	ORC_UNIQUE_PTR<InputStream> inStream = readLocalFile(input_file);
	ReaderOptions readerOpts;
	ORC_UNIQUE_PTR<Reader> reader = createReader(std::move(inStream), readerOpts);

	// Get the number of stripes in the file
	const uint64_t num_stripes = reader->getNumberOfStripes();

	uint64_t total_num_rows = 0;
	for (uint64_t s = 0; s < num_stripes; s++) {
		total_num_rows += reader->getStripe(s)->getNumberOfRows();
	}


	// Don't make more threads than there are rows
	uint64_t active_threads = total_num_rows;
	struct thread_args *thread_args = (struct thread_args *)malloc(sizeof(struct thread_args) * active_threads);
	pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * active_threads);

	std::cout << "Num stripes: " << num_stripes << "\n";
	std::cout << "Num threads: " << active_threads << "\n";
	std::cout << "Num rows: " << total_num_rows << "\n";

	// Assign work to each thread, one row per thread for now
	for (uint64_t i = 0; i < total_num_rows; i++) {
		struct thread_args *args = &thread_args[i];
		args->thread_num = i;
		args->filename = input_file;

		args->row_number i;
		args->sum = 0;
	}

	// Start each thread
	for (uint64_t i = 0; i < active_threads; i++) {
		if (pthread_create(&threads[i], NULL, &read_thread, &thread_args[i]) != 0) {
			std::cout << "Pthread create error\n";
			exit(1);
		}
	}

	// Wait for each thread
	uint64_t total_sum = 0;
	for (uint64_t i = 0; i < active_threads; i++) {
		pthread_join(threads[i], NULL);
		total_sum += thread_args[i].sum;
	}
	std::cout << "Sum first col: " << total_sum << "\n";		

	free(thread_args);
	free(threads);	

#if USE_PIM
	pim_deinit();
#endif
	return 0;
}
