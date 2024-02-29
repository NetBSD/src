#!/bin/sh

#set -x

cd "$(dirname "$0")"

log=/tmp/log
rm $log 2> /dev/null

for t in *.out
do
	/bin/sh checkone.sh $t
	if [ $? -ne 0 ]; then
		echo `basename $t` >> $log
	fi
done
