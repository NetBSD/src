#!/bin/sh

#set -x

if [ $# -ne 1 ]; then
	echo "usage: $0 config-name" >&2
	exit 1
fi

file=$1

cd "$(dirname "$0")"

trail=$(expr $file : ".*6$")
options=""
if [ $trail -eq 0 ]; then
	options="-4"
else
	options="-6"
fi

out=/tmp/$file.out$$

../../keama $options -N -i  $file.conf -o $out >&2
status=$?
if [ $status -eq 255 ]; then
	echo "$file config raised an error" >&2
	exit 1
fi

expected=$file.json

diff --brief $out $expected
if [ $? -ne 0 ]; then
	echo "$file config doesn't provide expected output" >&2
	exit 1
fi

exit $status
