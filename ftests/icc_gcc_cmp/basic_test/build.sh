#!/bin/sh

gdc -O3 -march=native test.d -o test_gdc
dmd test_with_mixin_template.d -O -release -allinst -inline
g++ -O3 -march=native test.cpp -o test_gcc
/opt/intel/composer_xe_2013_sp1.1.106/bin/ia32/icc test.cpp -O3 -march=native -o test_icc
