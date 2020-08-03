#!/bin/sh

#set -x

if [ $# -ne 1 ]; then
	echo "usage: $0 test-name" >&2
	exit 1
fi

if [ x$KEA4 = x ]; then
	echo "KEA4 is not set" >&2
fi
if [ x$KEA6 = x ]; then
	echo "KEA6 is not set" >&2
fi

file=$1

cd "$(dirname "$0")"

isout=$(expr $file : ".*\.out")
if [ $isout -eq 0 ]; then
	full=$file.out
else
	full=$file
fi
if [ ! -f $full ]; then
	echo "can't find $file" >&2
	exit 1
fi

is4=$(expr $file : ".*4")
is6=$(expr $file : ".*6")
if [ \( $is4 -eq 0 \) -a \( $is6 -eq 0 \) ]; then
	echo "can't get version from $file" >&2
	exit 1
fi

base=$(basename $full .out)
log=/tmp/$base.log$$
if [ $is4 -ne 0 ]; then
	$KEA4 -t $full >& $log
	if [ $? -ne 0 ]; then
		echo "$full raised an error" >&2
		exit 1
	fi
fi
if [ $is6 -ne 0 ]; then
	$KEA6 -t $full >& $log
	if [ $? -ne 0 ]; then
		echo "$full raised an error" >&2
		exit 1
	fi
fi
