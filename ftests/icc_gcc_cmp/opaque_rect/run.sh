#!/bin/bash

echo -ne "++ubyte\n\ngcc:\n"
time ./test_gcc_uint8
echo -ne "\n\n"icc:
time ./test_icc_uint8
echo -ne "\n\n"gdc:
time ./test_gdc_ubyte
echo -ne "\n\n"dmd:
time ./test_dmd_ubyte

echo '------------------'

echo -ne "++uint\n\ngcc:\n"
time ./test_gcc_uint
echo -ne "\n\n"icc:
time ./test_icc_uint
echo -ne "\n\n"gdc:
time ./test_gdc_uint
echo -ne "\n\n"dmd:
time ./test_dmd_uint
