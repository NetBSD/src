#!/bin/sh
# tests for TSIG-GSS updates

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0

DIGOPTS="@10.53.0.1 -p 5300"

# we don't want a KRB5_CONFIG setting breaking the tests
KRB5_CONFIG=/dev/null
export KRB5_CONFIG

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
    $NSUPDATE -g ns1/update.txt > /dev/null 2>&1 || {
	echo "I:update failed for $host $type $cmd"
	return 1
    }

    out=`$DIG $DIGOPTS -t $type -q $host | egrep "^${host}"`
    lines=`echo "$out" | grep "$digout" | wc -l`
    [ $lines -eq 1 ] || {
	echo "I:dig output incorrect for $host $type $cmd: $out"
	return 1
    }
    return 0
}

echo "I:testing updates as administrator"
KRB5CCNAME="FILE:"`pwd`/ns1/administrator.ccache
export KRB5CCNAME

test_update testdc1.example.nil. A "86400 A 10.53.0.10" "10.53.0.10" || status=1
test_update testdc2.example.nil. A "86400 A 10.53.0.11" "10.53.0.11" || status=1
test_update denied.example.nil. TXT "86400 TXT helloworld" "helloworld" && status=1

echo "I:testing updates as a user"
KRB5CCNAME="FILE:"`pwd`/ns1/testdenied.ccache
export KRB5CCNAME

test_update testdenied.example.nil. A "86400 A 10.53.0.12" "10.53.0.12" && status=1
test_update testdenied.example.nil. TXT "86400 TXT helloworld" "helloworld" || status=1

echo "I:testing external update policy"
test_update testcname.example.nil. TXT "86400 CNAME testdenied.example.nil" "testdenied" && status=1
perl ./authsock.pl --type=CNAME --path=ns1/auth.sock --pidfile=authsock.pid --timeout=120 > /dev/null 2>&1 &
sleep 1
test_update testcname.example.nil. TXT "86400 CNAME testdenied.example.nil" "testdenied" || status=1
test_update testcname.example.nil. TXT "86400 A 10.53.0.13" "10.53.0.13" && status=1

echo "I:testing external policy with SIG(0) key"
ret=0
$NSUPDATE -R random.data -k ns1/Kkey.example.nil.*.private <<END > /dev/null 2>&1 || ret=1
server 10.53.0.1 5300
zone example.nil
update add fred.example.nil 120 cname foo.bar.
send
END
output=`$DIG $DIGOPTS +short cname fred.example.nil.`
[ -n "$output" ] || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

[ $status -eq 0 ] && echo "I:tsiggss tests all OK"

kill `cat authsock.pid`
exit $status
