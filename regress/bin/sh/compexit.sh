#!/bin/sh
# The standard
# http://www.opengroup.org/onlinepubs/007904975/utilities/set.html
# says:
# 
# -e
# 
# When this option is on, if a simple command fails for any of the
# reasons listed in Consequences of Shell Errors or returns an exit
# status value >0, and is not part of the compound list following a
# while, until, or if keyword, and is not a part of an AND or OR list,
# and is not a pipeline preceded by the !  reserved word, then the shell
# shall immediately exit. 


check() {
	if [ "$1" != "$2" ]
	then
		echo "expected [$2], found [$1]" 1>&2
		exit 1
	fi
}

crud() {
	set -e
	for x in a
	do
		BAR="foo"
		false && echo true
		echo mumble
	done
}

foo=`crud`
check x$foo "xmumble"
