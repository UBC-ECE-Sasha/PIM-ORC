#!/bin/bash

for row in 1000 100 50 25 20 15 10 7 6 5
do
	echo $row
	/usr/bin/time ../orc-parser/reader -f ../../tpcds-data-orc32/store_sales/part-00000-ccb53a96-fab5-4acb-8ffa-ae6a728da779-c000.snappy.orc -t $row > store_sales10_32KB_$row.log
done

echo "All done"
