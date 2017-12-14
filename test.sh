#!/bin/bash
touch test_output.txt
for i in `seq 1 300`; do
	echo $(./threadpool) >> test_output.txt
done
