#!/bin/sh
#
# Test dd(1) functionality and bugs
#

test_dd_length() {
	result=$1
	cmd=$2
	set -- x `eval $cmd | wc -c`
	res=$2
	if [ x"$res" != x"$result" ]; then
		echo "Expected $result bytes of output, got $res: $cmd"
		exit 1
	fi
}

# Test for result messages accidentally pumped into the output file if
# the standard IO descriptors are closed.  The last of the three
# following tests is the one expected to fail. (NetBSD PR bin/8521)
#
test_dd_length 512 "dd if=/dev/zero of=/dev/fd/5 count=1 5>&1 >/dev/null 2>/dev/null"
test_dd_length 512 "dd if=/dev/zero of=/dev/fd/5 count=1 5>&1 >&- 2>/dev/null"
test_dd_length 512 "dd if=/dev/zero of=/dev/fd/5 count=1 5>&1 >&- 2>&-"

exit 0
