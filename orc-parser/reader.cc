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
	uint64_t start_row_number;
	uint64_t num_rows; // number of rows assigned to this thread
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
	uint64_t batch_size = reader->getRowIndexStride();
	ORC_UNIQUE_PTR<ColumnVectorBatch> batch = rowReader->createRowBatch(batch_size);

	// Seek to this thread's row
	rowReader->seekToRow(args->start_row_number);

	StructVectorBatch *root = dynamic_cast<StructVectorBatch *>(batch.get());
	LongVectorBatch *first_col = dynamic_cast<LongVectorBatch *>(root->fields[0]); // Get first column

	// read the rows
	uint64_t row_index_nums = args->num_rows / batch_size;
	if (args->num_rows % batch_size != 0)
		row_index_nums ++;	
	for (uint64_t row_index = 0; row_index < row_index_nums; row_index++) {
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
	uint64_t rows_per_thread = 10000;

	while ((opt = getopt(argc, argv, "f:t:")) != -1) {
		switch(opt) {
			case 'f':
				input_file = optarg;
				break;
			case 't':
				rows_per_thread = rows_per_thread * atoi(optarg);
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
	uint64_t active_threads = 0;
	for (uint64_t s = 0; s < num_stripes; s++) {
		uint64_t num_rows = reader->getStripe(s)->getNumberOfRows();
		active_threads += num_rows/ rows_per_thread;
		if (num_rows % rows_per_thread != 0)
			// each row should only cover one stripe, no thread with overlapping stripe
			active_threads ++;
	}

	struct thread_args *thread_args = (struct thread_args *)malloc(sizeof(struct thread_args) * active_threads);
	pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * active_threads);

	// check how many rows belong to each stripe
	std::cout << "Num stripes: " << num_stripes << "\n";
	std::cout << "Num threads: " << active_threads << "\n";

	uint64_t th = 0;
	uint64_t start_row_number = 0;
	for (uint64_t i=0; i<num_stripes; i++){
		uint64_t row_number = reader->getStripe(i)->getNumberOfRows();
		// assign rows of only one stripe to each thread, no thread should hold rows of multiple stripes
		while (row_number > 0) {
			struct thread_args *args = &thread_args[th];
			args->thread_num = th;
			args->filename = input_file;
			args->start_row_number = start_row_number;
			args->sum = 0;
			if (row_number >= rows_per_thread){
				args->num_rows = rows_per_thread;
				start_row_number += rows_per_thread;
				row_number -= rows_per_thread;
			} else {
				args->num_rows = row_number;
				start_row_number += row_number;
				row_number = 0;
			}
			th ++;		
		}
	
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
