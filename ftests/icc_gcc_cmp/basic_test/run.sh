#!/bin/bash

for i in {1..16} ; do
	echo gcc:
	time ./test_gcc $i;
	echo -e "\n\n"icc:
	time ./test_icc $i;
	echo -e "\n\n"gdc:
	time ./test_gdc $i;
	echo -e "\n\n"dmd:
	time ./test_with_mixin_template $i;
	echo '------------'
done
