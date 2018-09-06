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

checkout() {
	case $? in
	0) : ok ;;
	*) echo_i "failed"
	   status=`expr $status + 1`
	   return 1 ;;
	esac
	case $out in
	*$hash*) : ok ;;
	*) echo_i "expect $hash"
	   echo_i "output $out"
	   echo_i "failed"
	   status=`expr $status + 1` ;;
	esac
}

# test cases taken from RFC 5155 appendix A
algo=1 flags=0 iters=12 salt="aabbccdd"
while	read name hash
do
	echo_i "checking $NSEC3HASH $name"
	out=`$NSEC3HASH $salt $algo $iters $name`
	checkout

	echo_i "checking $NSEC3HASH -r $name"
	out=`$NSEC3HASH -r $algo $flags $iters $salt $name`
	checkout

done <<EOF
*.w.example R53BQ7CC2UVMUBFU5OCMM6PERS9TK9EN
2t7b4g4vsa5smi47k61mv5bv1a22bojr.example KOHAR7MBB8DC2CE8A9QVL8HON4K53UHI
a.example 35MTHGPGCU1QG68FAB165KLNSNK3DPVL
ai.example GJEQE526PLBF1G8MKLP59ENFD789NJGI
example 0P9MHAVEQVM6T7VBL5LOP2U3T2RP3TOM
ns1.example 2T7B4G4VSA5SMI47K61MV5BV1A22BOJR
ns2.example Q04JKCEVQVMU85R014C7DKBA38O0JI5R
w.example K8UDEMVP1J2F7EG6JEBPS17VP3N8I58H
x.w.example B4UM86EGHHDS6NEA196SMVMLO4ORS995
x.y.w.example 2VPTU5TIMAMQTTGL4LUU9KG21E0AOR3S
xx.example T644EBQK9BIBCNA874GIVR6JOJ62MLHV
y.w.example JI6NEOAEPV8B5O6K4EV33ABHA8HT9FGC
EOF

# test empty salt
checkempty() {
	hash=CK0POJMG874LJREF7EFN8430QVIT8BSM checkout &&
	hash=- checkout
}
name=com algo=1 flags=1 iters=0
echo_i "checking $NSEC3HASH '' $name"
out=`$NSEC3HASH '' $algo $iters $name`
checkempty
echo_i "checking $NSEC3HASH - $name"
out=`$NSEC3HASH - $algo $iters $name`
checkempty
echo_i "checking $NSEC3HASH -- '' $name"
out=`$NSEC3HASH -- '' $algo $iters $name`
checkempty
echo_i "checking $NSEC3HASH -- - $name"
out=`$NSEC3HASH -- - $algo $iters $name`
checkempty
echo_i "checking $NSEC3HASH -r '' $name"
out=`$NSEC3HASH -r $algo $flags $iters '' $name`
checkempty
echo_i "checking $NSEC3HASH -r - $name"
out=`$NSEC3HASH -r $algo $flags $iters - $name`
checkempty

checkfail() {
	case $? in
	0) echo_i "failed to fail"
	   status=`expr $status + 1`
	   return 1 ;;
	esac
}
echo_i "checking $NSEC3HASH missing args"
out=`$NSEC3HASH 00 1 0 2>&1`
checkfail
echo_i "checking $NSEC3HASH extra args"
out=`$NSEC3HASH 00 1 0 two names 2>&1`
checkfail
echo_i "checking $NSEC3HASH bad option"
out=`$NSEC3HASH -? 2>&1`
checkfail

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
