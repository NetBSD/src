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

test_dd_io() {
	res="`echo -n "$2" | eval $1`"
        if [ x"$res" != x"$3" ]; then
		echo "Expected \"$3\", got \"$res\": $1"
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

test_dd_io "dd conv=ucase 2>/dev/null" "TESTING upCase"     "TESTING UPCASE"
test_dd_io "dd conv=lcase 2>/dev/null" "testing DOWNcASE"    "testing downcase"

i=1
while [ $i -lt 1024 ]; do
  test_dd_io "dd ibs=$i conv=ucase 2>/dev/null" "TESTING upCase"     "TESTING UPCASE"
  test_dd_io "dd ibs=$i conv=lcase 2>/dev/null" "testing DOWNcASE"   "testing downcase"
  test_dd_io "dd obs=$i conv=ucase 2>/dev/null" "TESTING upCase"     "TESTING UPCASE"
  test_dd_io "dd obs=$i conv=lcase 2>/dev/null" "testing DOWNcASE"   "testing downcase"
  test_dd_io "dd bs=$i conv=ucase 2>/dev/null" "TESTING upCase"     "TESTING UPCASE"
  test_dd_io "dd bs=$i conv=lcase 2>/dev/null" "testing DOWNcASE"   "testing downcase"
  i=$(($i+1))
done

test_dd_io "dd conv=swab 2>/dev/null"  "testing byte swap"  "ettsni gybets awp"
test_dd_io "dd conv=swab 2>/dev/null"  "testing byte swaps" "ettsni gybets awsp"
i=2
while [ $i -lt 1024 ]; do
  test_dd_io "dd ibs=$i conv=swab 2>/dev/null"  "testing byte swap"  "ettsni gybets awp"
  test_dd_io "dd ibs=$i conv=swab 2>/dev/null"  "testing byte swaps" "ettsni gybets awsp"
  i=$(($i+2))
done

test_dd_io "dd ibs=1 conv=swab 2>/dev/null" "testing byte swap"  "testing byte swap"
test_dd_io "dd ibs=1 conv=swab 2>/dev/null" "testing byte swaps" "testing byte swaps"
test_dd_io "dd ibs=3 conv=swab 2>/dev/null" "testing byte swap"  "etsitn gbtyes wpa"
test_dd_io "dd ibs=3 conv=swab 2>/dev/null" "testing byte swaps" "etsitn gbtyes wpas"
test_dd_io "dd ibs=5 conv=swab 2>/dev/null" "testing byte swap"  "ettsignb yets wpa"
test_dd_io "dd ibs=5 conv=swab 2>/dev/null" "testing byte swaps" "ettsignb yets wpas"
test_dd_io "dd ibs=7 conv=swab 2>/dev/null" "testing byte swap"   "ettsnigb ty esawp"
test_dd_io "dd ibs=7 conv=swab 2>/dev/null" "testing byte swaps"  "ettsnigb ty esawsp"
test_dd_io "dd ibs=11 conv=swab 2>/dev/null" "testing byte swap"  "ettsni gybt ewspa"
test_dd_io "dd ibs=11 conv=swab 2>/dev/null" "testing byte swaps" "ettsni gybt ewspas"
test_dd_io "dd ibs=13 conv=swab 2>/dev/null" "testing byte swap"  "ettsni gybet wspa"
test_dd_io "dd ibs=13 conv=swab 2>/dev/null" "testing byte swaps" "ettsni gybet wspas"
test_dd_io "dd ibs=17 conv=swab 2>/dev/null" "testing byte swap"  "ettsni gybets awp"
test_dd_io "dd ibs=17 conv=swab 2>/dev/null" "testing byte swaps" "ettsni gybets awps"
test_dd_io "dd ibs=19 conv=swab 2>/dev/null" "testing byte swap"  "ettsni gybets awp"
test_dd_io "dd ibs=19 conv=swab 2>/dev/null" "testing byte swaps" "ettsni gybets awsp"

i=2
while [ $i -lt 1024 ]; do
  test_dd_io "dd bs=$i conv=swab 2>/dev/null"  "testing byte swap"  "ettsni gybets awp"
  test_dd_io "dd bs=$i conv=swab 2>/dev/null"  "testing byte swaps" "ettsni gybets awsp"
  i=$(($i+2))
done

test_dd_io "dd bs=1 conv=swab 2>/dev/null" "testing byte swap"  "testing byte swap"
test_dd_io "dd bs=1 conv=swab 2>/dev/null" "testing byte swaps" "testing byte swaps"
test_dd_io "dd bs=3 conv=swab 2>/dev/null" "testing byte swap"  "etsitn gbtyes wpa"
test_dd_io "dd bs=3 conv=swab 2>/dev/null" "testing byte swaps" "etsitn gbtyes wpas"
test_dd_io "dd bs=5 conv=swab 2>/dev/null" "testing byte swap"  "ettsignb yets wpa"
test_dd_io "dd bs=5 conv=swab 2>/dev/null" "testing byte swaps" "ettsignb yets wpas"
test_dd_io "dd bs=7 conv=swab 2>/dev/null" "testing byte swap"   "ettsnigb ty esawp"
test_dd_io "dd bs=7 conv=swab 2>/dev/null" "testing byte swaps"  "ettsnigb ty esawsp"
test_dd_io "dd bs=11 conv=swab 2>/dev/null" "testing byte swap"  "ettsni gybt ewspa"
test_dd_io "dd bs=11 conv=swab 2>/dev/null" "testing byte swaps" "ettsni gybt ewspas"
test_dd_io "dd bs=13 conv=swab 2>/dev/null" "testing byte swap"  "ettsni gybet wspa"
test_dd_io "dd bs=13 conv=swab 2>/dev/null" "testing byte swaps" "ettsni gybet wspas"
test_dd_io "dd bs=17 conv=swab 2>/dev/null" "testing byte swap"  "ettsni gybets awp"
test_dd_io "dd bs=17 conv=swab 2>/dev/null" "testing byte swaps" "ettsni gybets awps"
test_dd_io "dd bs=19 conv=swab 2>/dev/null" "testing byte swap"  "ettsni gybets awp"
test_dd_io "dd bs=19 conv=swab 2>/dev/null" "testing byte swaps" "ettsni gybets awsp"

i=1
while [ $i -lt 1024 ]; do
  test_dd_io "dd obs=$i conv=swab 2>/dev/null"  "testing byte swap"  "ettsni gybets awp"
  test_dd_io "dd obs=$i conv=swab 2>/dev/null"  "testing byte swaps" "ettsni gybets awsp"
  i=$(($i+1))
done

exit 0
