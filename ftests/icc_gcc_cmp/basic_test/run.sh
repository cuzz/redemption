#!/bin/bash

for i in {1..16} ; do
	echo gcc:
	time ./test_gcc $i;
	echo icc:
	time ./test_icc $i;
	echo gdc:
	time ./test_gdc $i;
	echo '------------'
done
