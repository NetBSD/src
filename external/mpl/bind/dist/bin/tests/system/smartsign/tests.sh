#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0

pzone=parent.nil
pfile=parent.db

czone=child.parent.nil
cfile=child.db

echo_i "generating child's keys"
# active zsk
czsk1=`$KEYGEN -q -a rsasha1 -r $RANDFILE -L 30 $czone`

# not yet published or active
czsk2=`$KEYGEN -q -a rsasha1 -r $RANDFILE -P none -A none $czone`

# published but not active
czsk3=`$KEYGEN -q -a rsasha1 -r $RANDFILE -A none $czone`

# inactive
czsk4=`$KEYGEN -q -a rsasha1 -r $RANDFILE -P now-24h -A now-24h -I now $czone`

# active in 12 hours, inactive 12 hours after that...
czsk5=`$KEYGEN -q -a rsasha1 -r $RANDFILE -P now+12h -A now+12h -I now+24h $czone`

# explicit successor to czk5
# (suppressing warning about lack of removal date)
czsk6=`$KEYGEN -q -r $RANDFILE -S $czsk5 -i 6h 2>/dev/null` 

# active ksk
cksk1=`$KEYGEN -q -a rsasha1 -r $RANDFILE -fk -L 30 $czone`

# published but not YET active; will be active in 20 seconds
cksk2=`$KEYGEN -q -a rsasha1 -r $RANDFILE -fk $czone`
# $SETTIME moved after other $KEYGENs

echo_i "revoking key"
# revoking key changes its ID
cksk3=`$KEYGEN -q -a rsasha1 -r $RANDFILE -fk $czone`
cksk4=`$REVOKE $cksk3`

echo_i "setting up sync key"
cksk5=`$KEYGEN -q -a rsasha1 -r $RANDFILE -fk -P now+1mo -A now+1mo -Psync now $czone`

echo_i "generating parent keys"
pzsk=`$KEYGEN -q -a rsasha1 -r $RANDFILE $pzone`
pksk=`$KEYGEN -q -a rsasha1 -r $RANDFILE -fk $pzone`

echo_i "setting child's activation time"
# using now+30s to fix RT 24561
$SETTIME -A now+30s $cksk2 > /dev/null

echo_i "signing child zone"
czoneout=`$SIGNER -Sg -e now+1d -X now+2d -r $RANDFILE -o $czone $cfile 2>&1`

echo_i "signing parent zone"
pzoneout=`$SIGNER -Sg -r $RANDFILE -o $pzone $pfile 2>&1`

czactive=`echo $czsk1 | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
czgenerated=`echo $czsk2 | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
czpublished=`echo $czsk3 | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
czinactive=`echo $czsk4 | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
czpredecessor=`echo $czsk5 | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
czsuccessor=`echo $czsk6 | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
ckactive=`echo $cksk1 | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
ckpublished=`echo $cksk2 | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
ckprerevoke=`echo $cksk3 | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
ckrevoked=`echo $cksk4 | sed 's/.*+005+0*\([0-9]*\)$/\1/'`

pzid=`echo $pzsk | sed 's/^K.*+005+0*\([0-9]\)/\1/'`
pkid=`echo $pksk | sed 's/^K.*+005+0*\([0-9]\)/\1/'`

echo_i "checking dnssec-signzone output matches expectations"
ret=0
echo "$pzoneout" | grep 'KSKs: 1 active, 0 stand-by, 0 revoked' > /dev/null || ret=1
echo "$pzoneout" | grep 'ZSKs: 1 active, 0 stand-by, 0 revoked' > /dev/null || ret=1
echo "$czoneout" | grep 'KSKs: 1 active, 1 stand-by, 1 revoked' > /dev/null || ret=1
echo "$czoneout" | grep 'ZSKs: 1 active, 2 stand-by, 0 revoked' > /dev/null || ret=1
if [ $ret != 0 ]; then
	echo_i "parent $pzoneout"
	echo_i "child $czoneout"
	echo_i "failed";
fi
status=`expr $status + $ret`

echo_i "rechecking dnssec-signzone output with -x"
ret=0
# use an alternate output file so -x doesn't interfere with later checks
pzoneout=`$SIGNER -Sxg -r $RANDFILE -o $pzone -f ${pfile}2.signed $pfile 2>&1`
czoneout=`$SIGNER -Sxg -e now+1d -X now+2d -r $RANDFILE -o $czone -f ${cfile}2.signed $cfile 2>&1`
echo "$pzoneout" | grep 'KSKs: 1 active, 0 stand-by, 0 revoked' > /dev/null || ret=1
echo "$pzoneout" | grep 'ZSKs: 1 active, 0 present, 0 revoked' > /dev/null || ret=1
echo "$czoneout" | grep 'KSKs: 1 active, 1 stand-by, 1 revoked' > /dev/null || ret=1
echo "$czoneout" | grep 'ZSKs: 1 active, 2 present, 0 revoked' > /dev/null || ret=1
if [ $ret != 0 ]; then
	echo_i "parent $pzoneout"
	echo_i "child $czoneout"
	echo_i "failed";
fi
status=`expr $status + $ret`

echo_i "checking parent zone DNSKEY set"
ret=0
grep "key id = $pzid" $pfile.signed > /dev/null || {
	ret=1
	echo_i "missing expected parent ZSK id = $pzid"
}
grep "key id = $pkid" $pfile.signed > /dev/null || {
	ret=1
	echo_i "missing expected parent KSK id = $pkid"
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking parent zone DS records"
ret=0
awk '$2 == "DS" {print $3}' $pfile.signed > dsset.out
grep -w "$ckactive" dsset.out > /dev/null || ret=1
grep -w "$ckpublished" dsset.out > /dev/null || ret=1
# revoked key should not be there, hence the &&
grep -w "$ckprerevoke" dsset.out > /dev/null && ret=1
grep -w "$ckrevoked" dsset.out > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking child zone DNSKEY set"
ret=0
grep "key id = $ckactive\$" $cfile.signed > /dev/null || {
	ret=1
	echo_i "missing expected child KSK id = $ckactive"
}
grep "key id = $ckpublished\$" $cfile.signed > /dev/null || {
	ret=1
	echo_i "missing expected child prepublished KSK id = $ckpublished"
}
grep "key id = $ckrevoked\$" $cfile.signed > /dev/null || {
	ret=1
	echo_i "missing expected child revoked KSK id = $ckrevoked"
}
grep "key id = $czactive\$" $cfile.signed > /dev/null || {
	ret=1
	echo_i "missing expected child ZSK id = $czactive"
}
grep "key id = $czpublished\$" $cfile.signed > /dev/null || {
	ret=1
	echo_i "missing expected child prepublished ZSK id = $czpublished"
}
grep "key id = $czinactive\$" $cfile.signed > /dev/null || {
	ret=1
	echo_i "missing expected child inactive ZSK id = $czinactive"
}
# should not be there, hence the &&
grep "key id = $ckprerevoke\$" $cfile.signed > /dev/null && {
	ret=1
	echo_i "found unexpect child pre-revoke ZSK id = $ckprerevoke"
}
grep "key id = $czgenerated\$" $cfile.signed > /dev/null && {
	ret=1
	echo_i "found unexpected child generated ZSK id = $czgenerated"
}
grep "key id = $czpredecessor\$" $cfile.signed > /dev/null && {
	echo_i "found unexpected ZSK predecessor id = $czpredecessor (ignored)"
}
grep "key id = $czsuccessor\$" $cfile.signed > /dev/null && {
	echo_i "found unexpected ZSK successor id = $czsuccessor (ignored)"
}
#grep "key id = $czpredecessor\$" $cfile.signed > /dev/null && ret=1
#grep "key id = $czsuccessor\$" $cfile.signed > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking key TTLs are correct"
ret=0
grep "${czone}. 30 IN" ${czsk1}.key > /dev/null 2>&1 || ret=1
grep "${czone}. 30 IN" ${cksk1}.key > /dev/null 2>&1 || ret=1
grep "${czone}. IN" ${czsk2}.key > /dev/null 2>&1 || ret=1
$SETTIME -L 45 ${czsk2} > /dev/null
grep "${czone}. 45 IN" ${czsk2}.key > /dev/null 2>&1 || ret=1
$SETTIME -L 0 ${czsk2} > /dev/null
grep "${czone}. IN" ${czsk2}.key > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking key TTLs were imported correctly"
ret=0
awk 'BEGIN {r = 0} $2 == "DNSKEY" && $1 != 30 {r = 1} END {exit r}' \
        ${cfile}.signed || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "re-signing and checking imported TTLs again"
ret=0
$SETTIME -L 15 ${czsk2} > /dev/null
czoneout=`$SIGNER -Sg -e now+1d -X now+2d -r $RANDFILE -o $czone $cfile 2>&1`
awk 'BEGIN {r = 0} $2 == "DNSKEY" && $1 != 15 {r = 1} END {exit r}' \
        ${cfile}.signed || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# There is some weirdness in Solaris 10 (Generic_120011-14), which
# is why the next section has all those echo $ret > /dev/null;sync
# commands
echo_i "checking child zone signatures"
ret=0
# check DNSKEY signatures first
awk '$2 == "RRSIG" && $3 == "DNSKEY" { getline; print $3 }' $cfile.signed > dnskey.sigs
sub=0
grep -w "$ckactive" dnskey.sigs > /dev/null || sub=1
if [ $sub != 0 ]; then echo_i "missing ckactive $ckactive (dnskey)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$ckrevoked" dnskey.sigs > /dev/null || sub=1
if [ $sub != 0 ]; then echo_i "missing ckrevoke $ckrevoke (dnskey)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$czactive" dnskey.sigs > /dev/null || sub=1
if [ $sub != 0 ]; then echo_i "missing czactive $czactive (dnskey)"; ret=1; fi
# should not be there:
echo $ret > /dev/null
sync
sub=0
grep -w "$ckprerevoke" dnskey.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found ckprerevoke $ckprerevoke (dnskey)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$ckpublished" dnskey.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found ckpublished $ckpublished (dnskey)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$czpublished" dnskey.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found czpublished $czpublished (dnskey)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$czinactive" dnskey.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found czinactive $czinactive (dnskey)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$czgenerated" dnskey.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found czgenerated $czgenerated (dnskey)"; ret=1; fi
# now check other signatures first
awk '$2 == "RRSIG" && $3 != "DNSKEY" && $3 != "CDNSKEY" && $3 != "CDS" { getline; print $3 }' $cfile.signed | sort -un > other.sigs
# should not be there:
echo $ret > /dev/null
sync
sub=0
grep -w "$ckactive" other.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found ckactive $ckactive (other)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$ckpublished" other.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found ckpublished $ckpublished (other)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$ckprerevoke" other.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found ckprerevoke $ckprerevoke (other)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$ckrevoked" other.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found ckrevoked $ckrevoked (other)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$czpublished" other.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found czpublished $czpublished (other)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$czinactive" other.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found czinactive $czinactive (other)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$czgenerated" other.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found czgenerated $czgenerated (other)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$czpredecessor" other.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found czpredecessor $czpredecessor (other)"; ret=1; fi
echo $ret > /dev/null
sync
sub=0
grep -w "$czsuccessor" other.sigs > /dev/null && sub=1
if [ $sub != 0 ]; then echo_i "found czsuccessor $czsuccessor (other)"; ret=1; fi
if [ $ret != 0 ]; then
    sed 's/^/I:dnskey sigs: /' < dnskey.sigs
    sed 's/^/I:other sigs: /' < other.sigs
    echo_i "failed";
fi
status=`expr $status + $ret`

echo_i "checking RRSIG expiry date correctness"
dnskey_expiry=`$CHECKZONE -o - $czone $cfile.signed 2> /dev/null |
              awk '$4 == "RRSIG" && $5 == "DNSKEY" {print $9; exit}' |
              cut -c1-10`
soa_expiry=`$CHECKZONE -o - $czone $cfile.signed 2> /dev/null |
           awk '$4 == "RRSIG" && $5 == "SOA" {print $9; exit}' |
           cut -c1-10`
[ $dnskey_expiry -gt $soa_expiry ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "waiting 30 seconds for key activation"
sleep 30
echo_i "re-signing child zone"
czoneout2=`$SIGNER -Sg -r $RANDFILE -o $czone -f $cfile.new $cfile.signed 2>&1`
mv $cfile.new $cfile.signed

echo_i "checking dnssec-signzone output matches expectations"
ret=0
echo "$czoneout2" | grep 'KSKs: 2 active, 0 stand-by, 1 revoked' > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking child zone signatures again"
ret=0
awk '$2 == "RRSIG" && $3 == "DNSKEY" { getline; print $3 }' $cfile.signed > dnskey.sigs
grep -w "$ckpublished" dnskey.sigs > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking sync record publication"
ret=0
grep -w CDNSKEY $cfile.signed > /dev/null || ret=1
grep -w CDS $cfile.signed > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking sync record deletion"
ret=0
$SETTIME -P now -A now -Dsync now ${cksk5} > /dev/null
$SIGNER -Sg -r $RANDFILE -o $czone -f $cfile.new $cfile.signed > /dev/null 2>&1
mv $cfile.new $cfile.signed
grep -w CDNSKEY $cfile.signed > /dev/null && ret=1
grep -w CDS $cfile.signed > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
