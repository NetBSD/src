#!/bin/ksh

function test_factor {
	echo "Testing: `eval factor $1`"
	res=`eval factor $1 2>&1`
	if [ "$res" != "$2" ]; then
		echo "Expected \"$2\", got \"$res\" from factor: `eval echo $1`"
		exit 1
	fi
}
	
# The first arg will get factor'd.
# The 2nd arg is an expected string/response from factor for that argument.

# Test overflow cases
test_factor '8675309' '8675309: 8675309'

exit 0
