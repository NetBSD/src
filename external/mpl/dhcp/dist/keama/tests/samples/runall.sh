#!/bin/sh

#set -x

cd "$(dirname "$0")"

for t in example simple test-a6 vmnet8
do
	echo $t
	/bin/sh runone.sh $t
done
