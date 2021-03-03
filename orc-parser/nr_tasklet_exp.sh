#!/bin/bash


tasklets_set="1 2 4 6 8 10 12 14 16 18 20 22"
file = "/home/tpcds-data-orc/scale=10-parts=1/part-00000-7cd0e244-020a-4d11-9b12-53eb404beac0-c000.snappy.orc"

#tasklets_set="14 16"





for tasklets in $tasklets_set; do
    # remove the dpu binary
    rm ../snappy/pim-snappy/decompress.dpu
    # compile with the given number of tasklets
    make USE_PIM=1 NR_TASKLETS=tasklets
    # run and store runtime 
    {echo tasklets; time ./reader -f file -t 8} 2>time.txt
done


