#
TEST=./gaitest
#TEST='./test -v'
#IF=`ifconfig -a | grep -v '^	' | sed -e 's/:.*//' | head -1 | awk '{print $1}'`

echo '== basic ones'
$TEST ::1 http
$TEST 127.0.0.1 http
$TEST localhost http
$TEST ::1 tftp
$TEST 127.0.0.1 tftp
$TEST localhost tftp
$TEST ::1 echo
$TEST 127.0.0.1 echo
$TEST localhost echo
echo

echo '== specific address family'
$TEST -4 localhost http
$TEST -6 localhost http
echo

echo '== empty hostname'
$TEST '' http
$TEST '' echo
$TEST '' tftp
$TEST '' 80
$TEST -P '' http
$TEST -P '' echo
$TEST -P '' tftp
$TEST -P '' 80
$TEST -S '' 80
$TEST -D '' 80
echo

echo '== empty servname'
$TEST ::1 ''
$TEST 127.0.0.1 ''
$TEST localhost ''
$TEST '' ''
echo

echo '== sock_raw'
$TEST -R -p 0 localhost ''
$TEST -R -p 59 localhost ''
$TEST -R -p 59 localhost 80
$TEST -R -p 59 localhost www
$TEST -R -p 59 ::1 ''
echo

echo '== unsupported family'
$TEST -f 99 localhost ''
echo

echo '== the following items are specified in jinmei scopeaddr format doc.'
$TEST fe80::1%lo0 http
#$TEST fe80::1%$IF http
echo
