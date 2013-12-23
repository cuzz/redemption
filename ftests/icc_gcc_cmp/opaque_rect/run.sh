#!/bin/bash

echo -n gcc:
time ./test_gcc;
echo -ne "\n\n"icc:
time ./test_icc;
echo -ne "\n\n"gdc:
time ./test_gdc;
echo -ne "\n\n"dmd:
time ./test_dmd;
