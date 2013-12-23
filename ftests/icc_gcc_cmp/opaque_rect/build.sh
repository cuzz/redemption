#!/bin/sh

gdc -O3 -march=native test_ubyte.d -o test_gdc_ubyte
gdc -O3 -march=native test_uint.d -o test_gdc_uint
dmd test_ubyte.d -O -release -oftest_dmd_ubyte
dmd test_uint.d -O -release -oftest_dmd_uint
g++ -O3 -march=native -DTYPEB=uint8_t test.cpp -o test_gcc_uint8
g++ -O3 -march=native -DTYPEB=unsigned test.cpp -o test_gcc_uint
/opt/intel/composer_xe_2013_sp1.1.106/bin/ia32/icc -DTYPEB=uint8_t test.cpp -O3 -march=native -o test_icc_uint8
/opt/intel/composer_xe_2013_sp1.1.106/bin/ia32/icc -DTYPEB=unsigned test.cpp -O3 -march=native -o test_icc_uint
