#!/bin/sh
#
# Copyright (C) 2011, 2012  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# Id: tests.sh,v 1.4 2011/06/10 01:32:37 each Exp 

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd"
DIGCMD="$DIG $DIGOPTS @10.53.0.2 -p 5300"
RNDCCMD="$RNDC -s 10.53.0.2 -p 9953 -c ../common/rndc.conf"

status=0

echo "I:preparing"
ret=0
$NSUPDATE -p 5300 -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text1.nil. 600 IN TXT "addition 1"
send
zone other.
update add text1.other. 600 IN TXT "addition 1"
send
END
[ -s ns2/nil.db.jnl ] || ret=1
[ -s ns2/other.db.jnl ] || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:rndc freeze"
$RNDCCMD freeze | sed 's/^/I:ns2 /'

echo "I:checking zone was dumped"
ret=0
grep "addition 1" ns2/nil.db > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking journal file is still present"
ret=0
[ -s ns2/nil.db.jnl ] || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking zone not writable"
ret=0
$NSUPDATE -p 5300 -k ns2/session.key > /dev/null 2>&1 <<END && ret=1
server 10.53.0.2
zone nil.
update add text2.nil. 600 IN TXT "addition 2"
send
END

$DIGCMD text2.nil. TXT | grep 'addition 2' >/dev/null && ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:rndc thaw"
$RNDCCMD thaw | sed 's/^/I:ns2 /'

echo "I:checking zone now writable"
ret=0
$NSUPDATE -p 5300 -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text3.nil. 600 IN TXT "addition 3"
send
END
$DIGCMD text3.nil. TXT | grep 'addition 3' >/dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:rndc sync"
ret=0
$RNDCCMD sync nil | sed 's/^/I:ns2 /'

echo "I:checking zone was dumped"
ret=0
grep "addition 3" ns2/nil.db > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking journal file is still present"
ret=0
[ -s ns2/nil.db.jnl ] || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking zone is still writable"
ret=0
$NSUPDATE -p 5300 -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text4.nil. 600 IN TXT "addition 4"
send
END

$DIGCMD text4.nil. TXT | grep 'addition 4' >/dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:rndc sync -clean"
ret=0
$RNDCCMD sync -clean nil | sed 's/^/I:ns2 /'

echo "I:checking zone was dumped"
ret=0
grep "addition 4" ns2/nil.db > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking journal file is deleted"
ret=0
[ -s ns2/nil.db.jnl ] && ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking zone is still writable"
ret=0
$NSUPDATE -p 5300 -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text5.nil. 600 IN TXT "addition 5"
send
END

$DIGCMD text4.nil. TXT | grep 'addition 4' >/dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking other journal files not removed"
ret=0
[ -s ns2/other.db.jnl ] || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:cleaning all zones"
$RNDCCMD sync -clean | sed 's/^/I:ns2 /'

echo "I:checking all journals removed"
ret=0
[ -s ns2/nil.db.jnl ] && ret=1
[ -s ns2/other.db.jnl ] && ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking that freezing static zones is not allowed"
ret=0
$RNDCCMD freeze static 2>&1 | grep 'not dynamic' > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking that journal is removed when serial is changed before thaw"
ret=0
sleep 1
$NSUPDATE -p 5300 -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone other.
update add text6.other. 600 IN TXT "addition 6"
send
END
[ -s ns2/other.db.jnl ] || ret=1
$RNDCCMD freeze other 2>&1 | sed 's/^/I:ns2 /'
serial=`awk '$3 == "serial" {print $1}' ns2/other.db`
newserial=`expr $serial + 1`
sed s/$serial/$newserial/ ns2/other.db > ns2/other.db.new
echo 'frozen TXT "frozen addition"' >> ns2/other.db.new
mv -f ns2/other.db.new ns2/other.db
$RNDCCMD thaw 2>&1 | sed 's/^/I:ns2 /'
sleep 1
[ -f ns2/other.db.jnl ] && ret=1
$NSUPDATE -p 5300 -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone other.
update add text7.other. 600 IN TXT "addition 7"
send
END
$DIGCMD text6.other. TXT | grep 'addition 6' >/dev/null || ret=1
$DIGCMD text7.other. TXT | grep 'addition 7' >/dev/null || ret=1
$DIGCMD frozen.other. TXT | grep 'frozen addition' >/dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking that journal is kept when ixfr-from-differences is in use"
ret=0
$NSUPDATE -p 5300 -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text6.nil. 600 IN TXT "addition 6"
send
END
[ -s ns2/nil.db.jnl ] || ret=1
$RNDCCMD freeze nil 2>&1 | sed 's/^/I:ns2 /'
serial=`awk '$3 == "serial" {print $1}' ns2/nil.db`
newserial=`expr $serial + 1`
sed s/$serial/$newserial/ ns2/nil.db > ns2/nil.db.new
echo 'frozen TXT "frozen addition"' >> ns2/nil.db.new
mv -f ns2/nil.db.new ns2/nil.db
$RNDCCMD thaw 2>&1 | sed 's/^/I:ns2 /'
sleep 1
[ -s ns2/nil.db.jnl ] || ret=1
$NSUPDATE -p 5300 -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text7.nil. 600 IN TXT "addition 7"
send
END
$DIGCMD text6.nil. TXT | grep 'addition 6' >/dev/null || ret=1
$DIGCMD text7.nil. TXT | grep 'addition 7' >/dev/null || ret=1
$DIGCMD frozen.nil. TXT | grep 'frozen addition' >/dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:test using second key"
ret=0
$RNDC -s 10.53.0.2 -p 9953 -c ns2/secondkey.conf status > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
exit $status
