# PIM-ORC
ORC file reader using PIM-assisted Snappy decompression. The program takes in an ORC file, reads through all the rows and sums the values in the first column, making calls to Snappy decompression where needed. The program can be built with or without PIM, the first option running the original Snappy implementation and the second running the DPU Snappy implementation.

## Build
To build the program, first enter the `orc-parser` directory. There are two arguments that may be passed to the `make` command:
* `USE_PIM`: Set to 1 to use the DPU implementation, default is 0 which uses the CPU implementation
* `NR_TASKLETS`: If using the DPU implementation set to a value less than or equal to 24 to set the number of tasklets on the DPU, default is 1

So if you wanted to use the DPUs with 5 tasklets, the command would be: `make USE_PIM=1 NR_TASKLETS=5`.

Note that due a bug in the build process (Issue #1), the build will fail the first time around if using PIM. Simply run the `make` command twice to get the build to succeed. 

If remaking with a different NR_TASKLETS, make sure to run `make clean` or simply delete `decompress.dpu` `make` to make sure the binary file is rebuilt with the new number of tasklets.

## Run
To execute the program, run the following command:
```
./reader -f [ORC input test file] -t [number of rows assigned to thread / 10000]
```
10000 is the size of the stride of the snappy reader. 
The program output should look something like this:
<img width="717" alt="orc-parser-output" src="https://user-images.githubusercontent.com/25714353/103724599-90008880-4f89-11eb-936a-2e2ed499f5e2.png">
