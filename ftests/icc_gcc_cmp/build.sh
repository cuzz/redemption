#!/bin/sh

[ -z "$*" ] && echo $0 'filename.cpp [g++-options] [icc-options]' >&2 && exit 1

cd "$(dirname $0)"
rdp_dir="$(pwd)/../.."

g++ \
-I "$rdp_dir"/core \
-I "$rdp_dir"/utils \
-I "$rdp_dir"/headers \
-I "$rdp_dir"/transport \
-I "$rdp_dir"/acl \
-I "$rdp_dir"/mod \
-I "$rdp_dir"/front \
-I "$rdp_dir"/capture \
-I "$rdp_dir"/regex \
-O3 -march=native -DNDEBUG \
$2 \
"$1" \
-lpng -lssl \
-o a.out \
&

/opt/intel/composer_xe_2013_sp1.1.106/bin/ia32/icc \
-I "$rdp_dir"/core \
-I "$rdp_dir"/utils \
-I "$rdp_dir"/headers \
-I "$rdp_dir"/transport \
-I "$rdp_dir"/acl \
-I "$rdp_dir"/mod \
-I "$rdp_dir"/front \
-I "$rdp_dir"/capture \
-I "$rdp_dir"/regex \
-O3 -march=native -DNDEBUG \
-fexceptions -std=c++11 \
$3 \
"$1" \
-lpng -lssl \
-o b.out 2> /dev/null

wait
