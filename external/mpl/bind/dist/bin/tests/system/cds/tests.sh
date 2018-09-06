#!/bin/sh -e
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
n=0
fail() {
	echo_i "failed"
	status=`expr $status + 1`
}

runcmd() {
        "$@" 1> out.$n 2> err.$n
	echo $?
}

testcase() {
	n=`expr $n + 1`
	echo_i "$name ($n)"
	expect=$1
	shift
	result=`runcmd "$@"`
	check_stdout
	check_stderr
	if [ "$expect" -ne "$result" ]; then
                echo "D:exit status does not match $expect"
		fail
	fi
        unset name err out
}

check_stderr() {
	if [ -n "${err:=}" ]; then
		egrep "$err" err.$n >/dev/null && return 0
	else
		[ -s err.$n ] || return 0
	fi
	echo "D:stderr did not match '$err'"
	sed 's/^/D:/' err.$n
	fail
}

check_stdout() {
	cmp out.$n "${out:-empty}" >/dev/null && return
	echo "D:stdout did not match '$out'"
	(	echo "wanted"
		cat "$out"
		echo "got"
		cat out.$n
	) | sed 's/^/D:/'
	fail
}

Z=cds.test

name='usage'
err='Usage'
testcase 1 $CDS

name='need a DS file'
err='DS pathname'
testcase 1 $CDS $Z

name='name of dsset in directory'
err="./dsset-$Z.: file not found"
testcase 1 $CDS -d . $Z

name='load a file'
err='could not find DS records'
testcase 1 $CDS -d empty $Z

name='load DS records'
err='path to file containing child data must be specified'
testcase 1 $CDS -d DS.1 $Z

name='missing DNSKEY'
err='could not find signed DNSKEY RRset'
testcase 1 $CDS -f db.null -d DS.1 $Z

name='sigs too old'
err='could not validate child DNSKEY RRset'
testcase 1 $CDS -f sig.null -d DS.1 $Z

name='sigs too old, verbosely'
err='skip RRSIG by key [0-9]+: too old'
testcase 1 $CDS -v1 -f sig.null -d DS.1 $Z

name='old sigs are allowed'
err='found RRSIG by key'
out=DS.1
testcase 0 $CDS -v1 -s -7200 -f sig.null -d DS.1 $Z

name='no CDS/CDNSKEY records'
out=DS.1
testcase 0 $CDS -s -7200 -f sig.null -d DS.1 $Z

name='no child records, verbosely'
err='has neither CDS nor CDNSKEY records'
out=DS.1
testcase 0 $CDS -v1 -s -7200 -f sig.null -d DS.1 $Z

name='unsigned CDS'
err='missing RRSIG CDS records'
testcase 1 $CDS -f brk.unsigned-cds -d DS.1 $Z

name='correct signature inception time'
$CDS -v3 -s -7200 -f sig.cds.1 -d DS.1 $Z 1>xout 2>xerr
testcase 0 $PERL checktime.pl 3600 xerr

name='in-place reads modification time'
testcase 0 $CDS -f sig.cds.1 -i.bak -d DS.inplace $Z

name='in-place output correct modification time'
testcase 0 $PERL checkmtime.pl 3600 DS.inplace

name='in-place backup correct modification time'
testcase 0 $PERL checkmtime.pl 7200 DS.inplace.bak

name='in-place correct output'
testcase 0 cmp DS.1 DS.inplace

name='in-place backup unmodified'
testcase 0 cmp DS.1 DS.inplace.bak

name='one mangled DS'
err='found RRSIG by key'
out=DS.1
testcase 0 $CDS -v1 -s -7200 -f sig.cds.1 -d DS.broke1 $Z

name='other mangled DS'
err='found RRSIG by key'
out=DS.1
testcase 0 $CDS -v1 -s -7200 -f sig.cds.1 -d DS.broke2 $Z

name='both mangled DS'
err='could not validate child DNSKEY RRset'
testcase 1 $CDS -v1 -s -7200 -f sig.cds.1 -d DS.broke12 $Z

name='mangle RRSIG CDS by ZSK'
err='found RRSIG by key'
out=DS.1
testcase 0 $CDS -v1 -s -7200 -f brk.rrsig.cds.zsk -d DS.1 $Z

name='mangle RRSIG CDS by KSK'
err='could not validate child CDS RRset'
testcase 1 $CDS -v1 -s -7200 -f brk.rrsig.cds.ksk -d DS.1 $Z

name='mangle CDS 1'
err='could not validate child DNSKEY RRset with new DS records'
testcase 1 $CDS -s -7200 -f sig.cds-mangled -d DS.1 $Z

name='inconsistent digests'
err='do not cover each key with the same set of digest types'
testcase 1 $CDS -s -7200 -f sig.bad-digests -d DS.1 $Z

name='inconsistent algorithms'
err='missing signature for algorithm'
testcase 1 $CDS -s -7200 -f sig.bad-algos -d DS.1 $Z

name='add DS records'
out=DS.both
$CDS -s -7200 -f sig.cds.both -d DS.1 $Z >DS.out
# sort to allow for numerical vs lexical order of key tags
testcase 0 sort DS.out

name='update add'
out=UP.add2
testcase 0 $CDS -u -s -7200 -f sig.cds.both -d DS.1 $Z

name='remove DS records'
out=DS.2
testcase 0 $CDS -s -7200 -f sig.cds.2 -d DS.both $Z

name='update del'
out=UP.del1
testcase 0 $CDS -u -s -7200 -f sig.cds.2 -d DS.both $Z

name='swap DS records'
out=DS.2
testcase 0 $CDS -s -7200 -f sig.cds.2 -d DS.1 $Z

name='update swap'
out=UP.swap
testcase 0 $CDS -u -s -7200 -f sig.cds.2 -d DS.1 $Z

name='TTL from -T'
out=DS.ttl2
testcase 0 $CDS -T 3600 -s -7200 -f sig.cds.2 -d DS.1 $Z

name='update TTL from -T'
out=UP.swapttl
testcase 0 $CDS -u -T 3600 -s -7200 -f sig.cds.2 -d DS.1 $Z

name='update TTL from dsset'
out=UP.swapttl
testcase 0 $CDS -u -s -7200 -f sig.cds.2 -d DS.ttl1 $Z

name='TTL from -T overrides dsset'
out=DS.ttlong2
testcase 0 $CDS -T 7200 -s -7200 -f sig.cds.2 -d DS.ttl1 $Z

name='stable DS record order (changes)'
out=DS.1
testcase 0 $CDS -s -7200 -f sig.cds.rev1 -d DS.2 $Z

name='CDNSKEY default algorithm'
out=DS.2-2
testcase 0 $CDS -s -7200 -f sig.cdnskey.2 -d DS.1 $Z

name='CDNSKEY SHA1'
out=DS.2-1
testcase 0 $CDS -a SHA1 -s -7200 -f sig.cdnskey.2 -d DS.1 $Z

name='CDNSKEY two algorithms'
out=DS.2
testcase 0 $CDS -a SHA1 -a SHA256 -s -7200 -f sig.cdnskey.2 -d DS.1 $Z

name='CDNSKEY two algorithms, reversed'
out=DS.2
testcase 0 $CDS -a SHA256 -a SHA1 -s -7200 -f sig.cdnskey.2 -d DS.1 $Z

name='CDNSKEY and CDS'
out=DS.2
testcase 0 $CDS -s -7200 -f sig.cds.cdnskey.2 -d DS.1 $Z

name='prefer CDNSKEY'
out=DS.2-2
testcase 0 $CDS -D -s -7200 -f sig.cds.cdnskey.2 -d DS.1 $Z

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
