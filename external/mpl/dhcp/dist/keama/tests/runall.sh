#!/bin/sh

#set -x

cd "$(dirname "$0")"

echo subdirs:
/bin/sh samples/runall.sh

echo tests:
for t in *.err* *.in*
do
	echo `basename $t`
	/bin/sh runone.sh $t
done
