#!/bin/sh
# tests for TSIG-GSS updates

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0

DIGOPTS="@10.53.0.1 -p 5300"

test_update() {
    host="$1"
    type="$2"
    cmd="$3"
    digout="$4"
    should_fail="$5"

    cat <<EOF > ns1/update.txt
server 10.53.0.1 5300
update add $host $cmd
send
EOF

    echo "I:testing update for $host $type $cmd $comment"
    $NSUPDATE -k ns1/ddns.key ns1/update.txt > /dev/null 2>&1 || {
	[ "$should_fail" ] || \
             echo "I:update failed for $host $type $cmd"
	return 1
    }

    out=`$DIG $DIGOPTS -t $type -q $host | egrep "^$host"`
    lines=`echo "$out" | grep "$digout" | wc -l`
    [ $lines -eq 1 ] || {
	[ "$should_fail" ] || \
            echo "I:dig output incorrect for $host $type $cmd: $out"
	return 1
    }
    return 0
}

ret=0

test_update testdc1.example.nil. A "86400 A 10.53.0.10" "10.53.0.10" || ret=1
status=`expr $status + $ret`

test_update testdc2.example.nil. A "86400 A 10.53.0.11" "10.53.0.11" || ret=1
status=`expr $status + $ret`

test_update testdc3.example.nil. A "86400 A 10.53.0.10" "10.53.0.10" || ret=1
status=`expr $status + $ret`

test_update deny.example.nil. TXT "86400 TXT helloworld" "helloworld" should_fail && ret=1
status=`expr $status + $ret`

echo "I:testing passing client info into DLZ driver"
ret=0
out=`$DIG $DIGOPTS +short -t txt -q source-addr.example.nil`
addr=`eval echo $out | cut -f1 -d'#'`
[ "$addr" = "10.53.0.1" ] || ret=1
[ "$ret" -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:testing DLZ driver is cleaned up on reload"
$RNDC -c ../common/rndc.conf -s 10.53.0.1 -p 9953 reload 2>&1 | sed 's/^/I:ns1 /'
for i in 0 1 2 3 4 5 6 7 8 9; do
    ret=0
    grep 'dlz_example: shutting down zone example.nil' ns1/named.run > /dev/null 2>&1 || ret=1
    [ "$ret" -eq 0 ] && break
done
[ "$ret" -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

exit $status
