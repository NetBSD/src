#!/bin/sh

targ=${1}

. ${2}

if [ X"$targ_underscore" = X"yes" ]; then
	UNDERSCORE=1
else
	UNDERSCORE=0
fi

echo "int prepends_underscore = ${UNDERSCORE};"
