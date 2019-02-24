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

$SHELL clean.sh

ln -s $CHECKZONE named-compilezone

# Test 1: KSK goes inactive before successor is active
dir=01-ksk-inactive
rm -f $dir/K*.key
rm -f $dir/K*.private
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`
$SETTIME -K $dir -I +9mo -D +1y $ksk1 > /dev/null 2>&1
ksk2=`$KEYGEN -q -K $dir -S $ksk1`
$SETTIME -K $dir -I +7mo $ksk1 > /dev/null 2>&1
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`

# Test 2: ZSK goes inactive before successor is active
dir=02-zsk-inactive
rm -f $dir/K*.key
rm -f $dir/K*.private
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`
$SETTIME -K $dir -I +9mo -D +1y $zsk1 > /dev/null 2>&1
zsk2=`$KEYGEN -q -K $dir -S $zsk1`
$SETTIME -K $dir -I +7mo $zsk1 > /dev/null 2>&1
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`

# Test 3: KSK is unpublished before its successor is published
dir=03-ksk-unpublished
rm -f $dir/K*.key
rm -f $dir/K*.private
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`
$SETTIME -K $dir -I +9mo -D +1y $ksk1 > /dev/null 2>&1
ksk2=`$KEYGEN -q -K $dir -S $ksk1`
$SETTIME -K $dir -D +6mo $ksk1 > /dev/null 2>&1
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`

# Test 4: ZSK is unpublished before its successor is published
dir=04-zsk-unpublished
rm -f $dir/K*.key
rm -f $dir/K*.private
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`
$SETTIME -K $dir -I +9mo -D +1y $zsk1 > /dev/null 2>&1
zsk2=`$KEYGEN -q -K $dir -S $zsk1`
$SETTIME -K $dir -D +6mo $zsk1 > /dev/null 2>&1
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`

# Test 5: KSK deleted and successor published before KSK is deactivated
# and successor activated.
dir=05-ksk-unpub-active
rm -f $dir/K*.key
rm -f $dir/K*.private
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`
$SETTIME -K $dir -I +9mo -D +8mo $ksk1 > /dev/null 2>&1
ksk2=`$KEYGEN -q -K $dir -S $ksk1`
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`

# Test 6: ZSK deleted and successor published before ZSK is deactivated
# and successor activated.
dir=06-zsk-unpub-active
rm -f $dir/K*.key
rm -f $dir/K*.private
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`
$SETTIME -K $dir -I +9mo -D +8mo $zsk1 > /dev/null 2>&1
zsk2=`$KEYGEN -q -K $dir -S $zsk1`
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`

# Test 7: KSK rolled with insufficient delay after prepublication.
dir=07-ksk-ttl
rm -f $dir/K*.key
rm -f $dir/K*.private
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`
$SETTIME -K $dir -I +9mo -D +1y $ksk1 > /dev/null 2>&1
ksk2=`$KEYGEN -q -K $dir -S $ksk1`
# allow only 1 day between publication and activation
$SETTIME -K $dir -P +269d $ksk2 > /dev/null 2>&1
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`

# Test 8: ZSK rolled with insufficient delay after prepublication.
dir=08-zsk-ttl
rm -f $dir/K*.key
rm -f $dir/K*.private
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`
$SETTIME -K $dir -I +9mo -D +1y $zsk1 > /dev/null 2>&1
zsk2=`$KEYGEN -q -K $dir -S $zsk1`
# allow only 1 day between publication and activation
$SETTIME -K $dir -P +269d $zsk2 > /dev/null 2>&1
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`

# Test 9: KSK goes inactive before successor is active, but checking ZSKs
dir=09-check-zsk
rm -f $dir/K*.key
rm -f $dir/K*.private
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`
$SETTIME -K $dir -I +9mo -D +1y $ksk1 > /dev/null 2>&1
ksk2=`$KEYGEN -q -K $dir -S $ksk1`
$SETTIME -K $dir -I +7mo $ksk1 > /dev/null 2>&1
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`

# Test 10: ZSK goes inactive before successor is active, but checking KSKs
dir=10-check-ksk
rm -f $dir/K*.key
rm -f $dir/K*.private
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`
$SETTIME -K $dir -I +9mo -D +1y $zsk1 > /dev/null 2>&1
zsk2=`$KEYGEN -q -K $dir -S $zsk1`
$SETTIME -K $dir -I +7mo $zsk1 > /dev/null 2>&1
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`

# Test 11: ZSK goes inactive before successor is active, but after cutoff
dir=11-cutoff
rm -f $dir/K*.key
rm -f $dir/K*.private
zsk1=`$KEYGEN -q -K $dir -a rsasha1 -3 example.com`
$SETTIME -K $dir -I +18mo -D +2y $zsk1 > /dev/null 2>&1
zsk2=`$KEYGEN -q -K $dir -S $zsk1`
$SETTIME -K $dir -I +16mo $zsk1 > /dev/null 2>&1
ksk1=`$KEYGEN -q -K $dir -a rsasha1 -3fk example.com`

# Test 12: Too early KSK deletion
dir=12-ksk-deletion
ksk1=`$KEYGEN -q -K $dir -f KSK -a 8 -b 2048 -I +40d -D +40d example.com`
ksk2=`$KEYGEN -q -K $dir -S $ksk1.key example.com`

# Test 13: check names with/without dots at the end
dir=13-dotted-dotless
zsk1=`$KEYGEN -q -K $dir -a rsasha256 one.example`
zsk2=`$KEYGEN -q -K $dir -a rsasha256 two.example`
