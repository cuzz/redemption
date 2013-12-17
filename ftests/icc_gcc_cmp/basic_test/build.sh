#!/bin/sh

gdc -O3 -march=native test.d -o test_gdc
g++ -O3 -march=native test.cpp -o test_gcc
/opt/intel/composer_xe_2013_sp1.1.106/bin/ia32/icc test.cpp -fast -march=native -o test_icc
