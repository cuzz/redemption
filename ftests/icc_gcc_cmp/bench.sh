#!/bin/bash

[ -z "$*" ] && files=*-gcc.out || files="$@"

for f in $files ; do
	echo -e ${f//-gcc.out} "\ngcc	icc"
	(./$f ; ./$f ; ./$f) > /tmp/gcc.txt ;
	f2=${f/gcc/icc}
	(./$f2 ; ./$f2 ; ./$f2) > /tmp/icc.txt ;
	paste /tmp/gcc.txt /tmp/icc.txt
	#paste <(./$f ; ./$f ; ./$f) <(./$f2 ; ./$f2 ; ./$f2)
	echo
done
