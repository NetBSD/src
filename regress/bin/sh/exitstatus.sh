#!/bin/sh

check() {
	if [ "$1" != "$2" ]
	then
		echo "expected [$2], found [$1]" 1>&2
		exit 1
	fi
}

crud() {

	test yes = no

	cat <<EOF
$?
EOF
}

foo=`crud`
check x$foo x1
