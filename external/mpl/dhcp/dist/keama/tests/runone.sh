#!/bin/sh

#set -x

if [ $# -ne 1 ]; then
	echo "usage: $0 test-name" >&2
	exit 1
fi

file=$1

cd "$(dirname "$0")"

isin=$(expr $file : ".*\.in*")
iserr=$(expr $file : ".*\.err*")
if [ \( $isin -eq 0 \) -a \( $iserr -eq 0 \) ]; then
	full=$file.in*
	if [ ! -f $full ]; then
		full=$file.err*
	fi
else
	full=$file
fi

if [ ! -f $full ]; then
	echo "can't find $file" >&2
	exit 1
fi

errcase=$(expr $full : ".*\.err*")

trail=
if [ $errcase -eq 0 ]; then
	trail=$(expr $full : ".*\.in\(.\)")
else
	trail=$(expr $full : ".*\.err\(.\)")
fi

options=""
dual=0
hook=0

case $trail in
	'') dual=1;;
	4) options="-4";;
	6) options="-6";;
	F) options="-4 -r fatal";;
	P) options="-4 -r pass";;
	d) options="-4 -D";;
	D) options="-6 -D";;
	n) options="-4 -N";;
	N) options="-6 -N";;
	l) options="-4 -l ${HOOK:-/path/}"; hook=1;;
	L) options="-6 -l ${HOOK:-/path/}"; hook=1;;
	*) echo "unrecognized trail '$trail' in '$full'" >&2; exit 1;;
esac

if [ $errcase -ne 0 ]; then
	base=$(basename $full .err$trail)
else
	if [ $dual -ne 0 ]; then
		echo "required trail ([45FP]) in '$full'" >&2
		exit 1
	fi
	base=$(basename $full .in$trail)
fi

out=/tmp/$base.out$$
expected=""
if [ $errcase -ne 0 ]; then
	expected=$base.msg
else
	expected=$base.out
fi

if [ $errcase -ne 0 ]; then
	if [ $dual -eq 1 ]; then
		../keama -4 -i $full >$out 2>&1
		if [ $? -ne 255 ]; then
			echo "$full -4 doesn't fail as expected" >&2
			exit 1
		fi
		../keama -6 -i $full >$out 2>&1 
		if [ $? -ne 255 ]; then
			echo "$full -6 doesn't fail as expected" >&2
			exit 1
		fi
	else
		../keama $options -i $full >$out 2>&1 
		if [ $? -ne 255 ]; then
			echo "$full doesn't fail as expected" >&2
			exit 1
		fi
	fi
else
	../keama $options -i  $full -o $out >&2
	if [ $? -eq 255 ]; then
		echo "$full raised an error" >&2
		exit 1
	fi
fi

if [ $hook -eq 1 ]; then
    sed s,/path/,${HOOK:-/path/}, < ${expected}L > $expected
fi

if [ $errcase -ne 0 ]; then
	cat $out | head -1 | diff --brief - $expected
	if [ $? -ne 0 ]; then
		echo "$full doesn't provide expected output" >&2
		exit 1
	fi
else
	diff --brief $out $expected
	if [ $? -ne 0 ]; then
		echo "$full doesn't provide expected output" >&2
		exit 1
	fi
fi
