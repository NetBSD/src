#!/bin/sh
# $NetBSD: set_e.sh,v 1.1 2005/03/31 08:52:49 yamt Exp $

rval=0
exec >&2

nl='
'
OIFS="$IFS"

check1()
{
	result="`eval $1`"
	# Remove newlines
	IFS="$nl"
	result="`echo $result`"
	IFS="$OIFS"
	if [ "$2" != "$result" ]
	then
		echo "command: $1"
		echo "    expected [$2], found [$result]"
		rval=1
	fi
}

p()
{

	if [ $? = 0 ]
	then
		echo ${1}0
	else
		echo ${1}1
	fi
}

check()
{
	check1 "((set -e; $1; p X); p Y)" "$2"
}

check 'exit 1' 'Y1'
check 'false' 'Y1'
check '(false)' 'Y1'
check 'false || false' 'Y1'
check '/nonexistent' 'Y1'

exit $rval
