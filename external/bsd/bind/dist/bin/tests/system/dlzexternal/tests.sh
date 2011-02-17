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

    cat <<EOF > ns1/update.txt
server 10.53.0.1 5300
update add $host $cmd
send
EOF
    echo "I:testing update for $host $type $cmd"
    $NSUPDATE -k ns1/ddns.key ns1/update.txt || {
	echo "I:update failed for $host $type $cmd"
	return 1
    }

    out=`$DIG $DIGOPTS -t $type -q $host | egrep ^$host`
    lines=`echo "$out" | grep "$digout" | wc -l`
    [ $lines -eq 1 ] || {
	echo "I:dig output incorrect for $host $type $cmd: $out"
	return 1
    }
    return 0
}

test_update testdc1.example.nil. A "86400 A 10.53.0.10" "10.53.0.10" || status=1
test_update testdc2.example.nil. A "86400 A 10.53.0.11" "10.53.0.11" || status=1
test_update testdc3.example.nil. A "86400 A 10.53.0.10" "10.53.0.10" || status=1
test_update deny.example.nil. TXT "86400 TXT helloworld" "helloworld" && status=1

[ $status -eq 0 ] && echo "I:dlzexternal tests all OK"

exit $status
