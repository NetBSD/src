#!/bin/ksh

function test_factor {
	echo "Testing: \`/usr/games/factor $1\`"
	res=`/usr/games/factor $1 2>&1`
	if [ "$res" != "$2" ]; then
		echo "Expected \"$2\", got \"$res\" from factor: `eval echo $1`"
		exit 1
	fi
}
	
# The first arg will get factor'd.
# The 2nd arg is an expected string/response from factor for that argument.

# Test overflow cases
test_factor '8675309' '8675309: 8675309'
test_factor '6172538568' '6172538568: 2 2 2 3 7 17 2161253'

exit 0
