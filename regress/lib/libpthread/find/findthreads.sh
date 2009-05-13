#!/bin/sh

s=0
while expr $s \< 100 >/dev/null; do
	./findthreads $s || exit 1
	s=`expr $s + 1`
done
