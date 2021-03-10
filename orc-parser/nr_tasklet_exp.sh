#!/bin/bash


tasklets_set="8 6 4 3 2 1"
file="/home/upmem0016/mojanjamalzadeh/tpcds-data-orc/scale=10-parts=1/part-00000-7cd0e244-020a-4d11-9b12-53eb404beac0-c000.snappy.orc"
for tasklets in $tasklets_set; do
    # remove the dpu binary
    rm ../snappy/pim-snappy/decompress.dpu
    # compile with the given number of tasklets
    make USE_PIM=1 NR_TASKLETS=$tasklets
    # run and store runtime
   echo "Tasklets: $tasklets"
   time ./reader -f ../testfiles/customer-scale1000-parts1.snappy.orc -t 100 
done


