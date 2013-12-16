#!/bin/bash

for f in *.cpp ; do
	./build.sh $f
	basefile=${f//.*}
	mv a.out $basefile-gcc.out
	mv b.out $basefile-icc.out
done
