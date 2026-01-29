#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

set -e

. ../conf.sh

DIGOPTS="+tcp +nosea +nostat +nocmd +norec +noques +noauth +noadd +nostats +dnssec -p ${PORT}"
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

check_zonestatus() (
  $RNDCCMD "10.53.0.$1" zonestatus -redirect >"zonestatus.out.ns$1.$n" \
    && grep "type: redirect" "zonestatus.out.ns$1.$n" >/dev/null \
    && grep "serial: 1" "zonestatus.out.ns$1.$n" >/dev/null
)

status=0
n=0

echo_i "checking normally loaded zone ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 a.normal.example a >dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n >/dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# When LMDB support is compiled in, this tests that migration from
# NZF to NZD occurs during named startup
echo_i "checking previously added zone ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 a.previous.example a >dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n >/dev/null || ret=1
grep '^a.previous.example' dig.out.ns2.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if $FEATURETEST --with-lmdb; then
  echo_i "checking that existing NZF file was renamed after migration ($n)"
  [ -e ns2/3bf305731dd26307.nzf~ ] || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

echo_i "adding new zone ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone 'added.example { type primary; file "added.db"; };' 2>&1 | sed 's/^/I:ns2 /'
_check_adding_new_zone() (
  $DIG $DIGOPTS @10.53.0.2 a.added.example a >dig.out.ns2.$n \
    && grep 'status: NOERROR' dig.out.ns2.$n >/dev/null \
    && grep '^a.added.example' dig.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_adding_new_zone || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null
echo_i "checking addzone errors are logged correctly"
ret=0
$RNDCCMD 10.53.0.2 addzone bad.example '{ type mister; };' 2>&1 | grep 'unexpected token' >/dev/null 2>&1 || ret=1
wait_for_log_peek 20 "addzone: 'mister' unexpected" ns2/named.run || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null
echo_i "checking modzone errors are logged correctly"
ret=0
$RNDCCMD 10.53.0.2 modzone added.example '{ type mister; };' 2>&1 | grep 'unexpected token' >/dev/null 2>&1 || ret=1
wait_for_log_peek 20 "modzone: 'mister' unexpected" ns2/named.run || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "adding a zone that requires quotes ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone '"32/1.0.0.127-in-addr.added.example" {
check-names ignore; type primary; file "added.db"; };' 2>&1 | sed 's/^/I:ns2 /'
_check_zone_that_requires_quotes() (
  $DIG $DIGOPTS @10.53.0.2 "a.32/1.0.0.127-in-addr.added.example" a >dig.out.ns2.$n \
    && grep 'status: NOERROR' dig.out.ns2.$n >/dev/null \
    && grep '^a.32/1.0.0.127-in-addr.added.example' dig.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_zone_that_requires_quotes || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "adding a zone with a quote in the name ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone '"foo\"bar.example" { check-names ignore; type primary; file "added.db"; };' 2>&1 | sed 's/^/I:ns2 /'
_check_zone_with_a_quote() (
  $DIG $DIGOPTS @10.53.0.2 "a.foo\"bar.example" a >dig.out.ns2.$n \
    && grep 'status: NOERROR' dig.out.ns2.$n >/dev/null \
    && grep '^a.foo\\"bar.example' dig.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_zone_with_a_quote || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "adding new zone with missing file ($n)"
ret=0
$DIG $DIGOPTS +all @10.53.0.2 a.missing.example a >dig.out.ns2.pre.$n || ret=1
grep "status: REFUSED" dig.out.ns2.pre.$n >/dev/null || ret=1
$RNDCCMD 10.53.0.2 addzone 'missing.example { type primary; file "missing.db"; };' 2>rndc.out.ns2.$n && ret=1
grep "file not found" rndc.out.ns2.$n >/dev/null || ret=1
$DIG $DIGOPTS +all @10.53.0.2 a.missing.example a >dig.out.ns2.post.$n || ret=1
grep "status: REFUSED" dig.out.ns2.post.$n >/dev/null || ret=1
digcomp dig.out.ns2.pre.$n dig.out.ns2.post.$n || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if ! $FEATURETEST --with-lmdb; then
  echo_i "verifying no comments in NZF file ($n)"
  ret=0
  hcount=$(grep "^# New zone file for view: _default" ns2/3bf305731dd26307.nzf | wc -l)
  [ $hcount -eq 0 ] || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

echo_i "checking rndc showzone with previously added zone ($n)"
ret=0
$RNDCCMD 10.53.0.2 showzone previous.example >rndc.out.ns2.$n
expected='zone "previous.example" { type primary; file "previous.db"; };'
[ "$(cat rndc.out.ns2.$n)" = "$expected" ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if $FEATURETEST --with-lmdb; then
  echo_i "checking zone is present in NZD ($n)"
  ret=0
  $NZD2NZF ns2/_default.nzd | grep previous.example >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

echo_i "deleting previously added zone ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone previous.example 2>&1 | sed 's/^/I:ns2 /'
_check_deleting_previously_added_zone() (
  $DIG $DIGOPTS @10.53.0.2 a.previous.example a >dig.out.ns2.$n \
    && grep 'status: REFUSED' dig.out.ns2.$n >/dev/null \
    && ! grep '^a.previous.example' dig.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_deleting_previously_added_zone || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

check_nzd2nzf() (
  $NZD2NZF ns2/_default.nzd >nzd2nzf.out.$n \
    && ! grep previous.example nzd2nzf.out.$n >/dev/null
)

if $FEATURETEST --with-lmdb; then
  echo_i "checking zone was deleted from NZD ($n)"
  retry_quiet 10 check_nzd2nzf || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

if ! $FEATURETEST --with-lmdb; then
  echo_i "checking NZF file now has comment ($n)"
  ret=0
  hcount=$(grep "^# New zone file for view: _default" ns2/3bf305731dd26307.nzf | wc -l)
  [ $hcount -eq 1 ] || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

echo_i "deleting newly added zone added.example ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone added.example 2>&1 | sed 's/^/I:ns2 /'
_check_deleting_newly_added_zone() (
  $DIG $DIGOPTS @10.53.0.2 a.added.example a >dig.out.ns2.$n \
    && grep 'status: REFUSED' dig.out.ns2.$n >/dev/null \
    && ! grep '^a.added.example' dig.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_deleting_newly_added_zone || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "deleting newly added zone with escaped quote ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone "foo\\\"bar.example" 2>&1 | sed 's/^/I:ns2 /'
_check_deleting_newly_added_zone_quote() (
  $DIG $DIGOPTS @10.53.0.2 "a.foo\"bar.example" a >dig.out.ns2.$n \
    && grep 'status: REFUSED' dig.out.ns2.$n >/dev/null \
    && ! grep "^a.foo\"bar.example" dig.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_deleting_newly_added_zone_quote || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking rndc showzone with a normally-loaded zone ($n)"
ret=0
$RNDCCMD 10.53.0.2 showzone normal.example >rndc.out.ns2.$n
expected='zone "normal.example" { type primary; file "normal.db"; };'
[ "$(cat rndc.out.ns2.$n)" = "$expected" ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking rndc showzone with a normally-loaded zone with trailing dot ($n)"
ret=0
$RNDCCMD 10.53.0.2 showzone finaldot.example >rndc.out.ns2.$n
expected='zone "finaldot.example." { type primary; file "normal.db"; };'
[ "$(cat rndc.out.ns2.$n)" = "$expected" ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking rndc showzone with a normally-loaded redirect zone ($n)"
ret=0
$RNDCCMD 10.53.0.1 showzone -redirect >rndc.out.ns1.$n
expected='zone "." { type redirect; file "redirect.db"; };'
[ "$(cat rndc.out.ns1.$n)" = "$expected" ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking rndc zonestatus with a normally-loaded redirect zone ($n)"
ret=0
$RNDCCMD 10.53.0.1 zonestatus -redirect >rndc.out.ns1.$n
grep "type: redirect" rndc.out.ns1.$n >/dev/null || ret=1
grep "serial: 0" rndc.out.ns1.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking rndc reload with a normally-loaded redirect zone ($n)"
ret=0
sleep 1
cp -f ns1/redirect.db.2 ns1/redirect.db
$RNDCCMD 10.53.0.1 reload -redirect >rndc.out.ns1.$n
retry_quiet 5 check_zonestatus 1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "delete a normally-loaded zone ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone normal.example >rndc.out.ns2.$n 2>&1
grep "is no longer active and will be deleted" rndc.out.ns2.$n >/dev/null || ret=11
grep "To keep it from returning when the server is restarted" rndc.out.ns2.$n >/dev/null || ret=1
grep "must also be removed from named.conf." rndc.out.ns2.$n >/dev/null || ret=1
_check_delete_normally_loaded_zone() (
  $DIG $DIGOPTS @10.53.0.2 a.normal.example a >dig.out.ns2.$n \
    && grep 'status: REFUSED' dig.out.ns2.$n >/dev/null
)
retry_quiet 5 _check_delete_normally_loaded_zone || ret=1

n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "attempting to add primary zone with inline signing ($n)"
$RNDCCMD 10.53.0.2 addzone 'inline.example { type primary; file "inline.db"; dnssec-policy default; inline-signing yes; };' 2>&1 | sed 's/^/I:ns2 /'
_check_add_primary_zone_with_inline() (
  $DIG $DIGOPTS @10.53.0.2 a.inline.example a >dig.out.ns2.$n \
    && grep 'status: NOERROR' dig.out.ns2.$n >/dev/null \
    && grep '^a.inline.example' dig.out.ns2.$n >/dev/null
)
retry_quiet 5 _check_add_primary_zone_with_inline || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "attempting to add primary zone with inline signing and missing file ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone 'inlinemissing.example { type primary; file "missing.db"; dnssec-policy default; inline-signing yes; };' 2>rndc.out.ns2.$n && ret=1
grep "file not found" rndc.out.ns2.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "attempting to add secondary zone with inline signing ($n)"
$RNDCCMD 10.53.0.2 addzone 'inlinesec.example { type secondary; primaries { 10.53.0.1; }; file "inlinesec.bk"; dnssec-policy default; inline-signing yes; };' 2>&1 | sed 's/^/I:ns2 /'
_check_add_secondary_with_inline() (
  $DIG $DIGOPTS @10.53.0.2 a.inlinesec.example a >dig.out.ns2.$n \
    && grep 'status: NOERROR' dig.out.ns2.$n >/dev/null \
    && grep '^a.inlinesec.example' dig.out.ns2.$n >/dev/null
)
retry_quiet 5 _check_add_secondary_with_inline || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "attempting to delete secondary zone with inline signing ($n)"
ret=0
retry_quiet 10 test -f ns2/inlinesec.bk.signed -a -f ns2/inlinesec.bk || ret=1
$RNDCCMD 10.53.0.2 delzone inlinesec.example >rndc.out2.test$n 2>&1 || ret=1
test -f inlinesec.bk \
  || grep '^inlinesec.bk$' rndc.out2.test$n >/dev/null || {
  echo_i "failed to report inlinesec.bk"
  ret=1
}
test ! -f inlinesec.bk.signed \
  || grep '^inlinesec.bk.signed$' rndc.out2.test$n >/dev/null || {
  echo_i "failed to report inlinesec.bk.signed"
  ret=1
}
n=$((n + 1))
status=$((status + ret))

echo_i "restoring secondary zone with inline signing ($n)"
$RNDCCMD 10.53.0.2 addzone 'inlinesec.example { type secondary; primaries { 10.53.0.1; }; file "inlinesec.bk"; dnssec-policy default; inline-signing yes; };' 2>&1 | sed 's/^/I:ns2 /'
_check_restoring_secondary_with_inline() (
  $DIG $DIGOPTS @10.53.0.2 a.inlinesec.example a >dig.out.ns2.$n \
    && grep 'status: NOERROR' dig.out.ns2.$n >/dev/null \
    && grep '^a.inlinesec.example' dig.out.ns2.$n >/dev/null
)
retry_quiet 5 _check_restoring_secondary_with_inline || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "deleting secondary zone with automatic zone file removal ($n)"
ret=0
retry_quiet 10 test -f ns2/inlinesec.bk.signed -a -f ns2/inlinesec.bk || ret=1
$RNDCCMD 10.53.0.2 delzone -clean inlinesec.example >/dev/null 2>&1
retry_quiet 10 test ! -f ns2/inlinesec.bk.signed -a ! -f ns2/inlinesec.bk
n=$((n + 1))
status=$((status + ret))

echo_i "modifying zone configuration ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone 'mod.example { type primary; file "added.db"; };' 2>&1 | sed 's/^/ns2 /' | cat_i
$DIG +norec $DIGOPTS @10.53.0.2 mod.example ns >dig.out.ns2.1.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.1.$n >/dev/null || ret=1
$RNDCCMD 10.53.0.2 modzone 'mod.example { type primary; file "added.db"; allow-query { none; }; };' 2>&1 | sed 's/^/ns2 /' | cat_i
$DIG +norec $DIGOPTS @10.53.0.2 mod.example ns >dig.out.ns2.2.$n || ret=1
$RNDCCMD 10.53.0.2 showzone mod.example | grep 'allow-query { "none"; };' >/dev/null 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that adding a 'stub' zone works ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone 'stub.example { type stub; primaries { 1.2.3.4; }; file "stub.example.bk"; };' >rndc.out.ns2.$n 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that adding a 'static-stub' zone works ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone 'static-stub.example { type static-stub; server-addresses { 1.2.3.4; }; };' >rndc.out.ns2.$n 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that adding a 'primary redirect' zone works ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone '"." { type redirect; file "redirect.db"; };' >rndc.out.ns2.$n 2>&1 || ret=1
_check_add_primary_redirect() (
  $RNDCCMD 10.53.0.2 showzone -redirect >showzone.out.ns2.$n 2>&1 \
    && grep "type redirect;" showzone.out.ns2.$n >/dev/null \
    && $RNDCCMD 10.53.0.2 zonestatus -redirect >zonestatus.out.ns2.$n 2>&1 \
    && grep "type: redirect" zonestatus.out.ns2.$n >/dev/null \
    && grep "serial: 0" zonestatus.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_add_primary_redirect || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that reloading a added 'primary redirect' zone works ($n)"
ret=0
sleep 1
cp -f ns2/redirect.db.2 ns2/redirect.db
$RNDCCMD 10.53.0.2 reload -redirect >rndc.out.ns2.$n
retry_quiet 10 check_zonestatus 2 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that retransfer of a added 'primary redirect' zone fails ($n)"
ret=0
$RNDCCMD 10.53.0.2 retransfer -redirect >rndc.out.ns2.$n 2>&1 && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that deleting a 'primary redirect' zone works ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone -redirect >rndc.out.ns2.$n 2>&1 || ret=1
_check_deleting_primary_redirect() (
  $RNDCCMD 10.53.0.2 showzone -redirect >showzone.out.ns2.$n 2>&1 || true
  grep 'not found' showzone.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_deleting_primary_redirect || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that adding a 'secondary redirect' zone works ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone '"." { type redirect; primaries { 10.53.0.3;}; file "redirect.bk"; };' >rndc.out.ns2.$n 2>&1 || ret=1
_check_adding_secondary_redirect() (
  $RNDCCMD 10.53.0.2 showzone -redirect >showzone.out.ns2.$n 2>&1 \
    && grep "type redirect;" showzone.out.ns2.$n >/dev/null \
    && $RNDCCMD 10.53.0.2 zonestatus -redirect >zonestatus.out.ns2.$n 2>&1 \
    && grep "type: redirect" zonestatus.out.ns2.$n >/dev/null \
    && grep "serial: 0" zonestatus.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_adding_secondary_redirect || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that retransfering a added 'secondary redirect' zone works ($n)"
ret=0
cp -f ns3/redirect.db.2 ns3/redirect.db
$RNDCCMD 10.53.0.3 reload . >showzone.out.ns3.$n 2>&1 || ret=1
_check_retransfering_secondary_redirect() (
  $RNDCCMD 10.53.0.2 retransfer -redirect >rndc.out.ns2.$n 2>&1 \
    && $RNDCCMD 10.53.0.2 zonestatus -redirect >zonestatus.out.ns2.$n 2>&1 \
    && grep "type: redirect" zonestatus.out.ns2.$n >/dev/null \
    && grep "serial: 1" zonestatus.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_retransfering_secondary_redirect || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that deleting a 'secondary redirect' zone works ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone -redirect >rndc.out.ns2.$n 2>&1 || ret=1
_check_deleting_secondary_redirect() (
  $RNDCCMD 10.53.0.2 showzone -redirect >showzone.out.ns2.$n 2>&1 || true
  grep 'not found' showzone.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_deleting_secondary_redirect || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that zone type 'hint' is properly rejected ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone '"." { type hint; file "hints.db"; };' >rndc.out.ns2.$n 2>&1 && ret=1
grep "zones not supported by addzone" rndc.out.ns2.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that zone type 'forward' is properly rejected ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone 'forward.example { type forward; forwarders { 1.2.3.4; }; forward only; };' >rndc.out.ns2.$n 2>&1 && ret=1
grep "zones not supported by addzone" rndc.out.ns2.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check that 'in-view' zones are properly rejected ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone 'in-view.example { in-view "_default"; };' >rndc.out.ns2.$n 2>&1 && ret=1
grep "zones not supported by addzone" rndc.out.ns2.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "reconfiguring server with multiple views"
cp ns2/named2.conf ns2/named.conf
rndc_reconfig ns2 10.53.0.2

echo_i "adding new zone to external view ($n)"
# NOTE: The internal view has "recursion yes" set, and so queries for
# nonexistent zones should return NOERROR.  The external view is
# "recursion no", so queries for nonexistent zones should return
# REFUSED.  This behavior should be the same regardless of whether
# the zone does not exist because a) it has not yet been loaded, b)
# it failed to load, or c) it has been deleted.
ret=0
$DIG +norec $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.added.example a >dig.out.ns2.intpre.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.intpre.$n >/dev/null || ret=1
$DIG +norec $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.extpre.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.extpre.$n >/dev/null || ret=1
$RNDCCMD 10.53.0.2 addzone 'added.example in external { type primary; file "added.db"; };' 2>&1 | sed 's/^/I:ns2 /'
$DIG +norec $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.added.example a >dig.out.ns2.int.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.int.$n >/dev/null || ret=1
$DIG +norec $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.ext.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.ext.$n >/dev/null || ret=1
grep '^a.added.example' dig.out.ns2.ext.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if ! $FEATURETEST --with-lmdb; then
  echo_i "checking new NZF file has comment ($n)"
  ret=0
  hcount=$(grep "^# New zone file for view: external" ns2/external.nzf | wc -l)
  [ $hcount -eq 1 ] || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

if $FEATURETEST --with-lmdb; then
  echo_i "verifying added.example in external view created an external.nzd DB ($n)"
  ret=0
  [ -e ns2/external.nzd ] || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

echo_i "checking rndc reload causes named to reload the external view's new zone config ($n)"
ret=0
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
_check_rndc_reload_external_view_config() (
  $DIG +norec $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.added.example a >dig.out.ns2.int.$n \
    && grep 'status: NOERROR' dig.out.ns2.int.$n >/dev/null \
    && $DIG +norec $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.ext.$n \
    && grep 'status: NOERROR' dig.out.ns2.ext.$n >/dev/null \
    && grep '^a.added.example' dig.out.ns2.ext.$n >/dev/null
)
retry_quiet 10 _check_rndc_reload_external_view_config || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking rndc showzone with newly added zone ($n)"
_check_rndc_showzone_newly_added() (
  if ! $FEATURETEST --with-lmdb; then
    expected='zone "added.example" in external { type primary; file "added.db"; };'
  else
    expected='zone "added.example" { type primary; file "added.db"; };'
  fi
  $RNDCCMD 10.53.0.2 showzone added.example in external >rndc.out.ns2.$n 2>/dev/null \
    && [ "$(cat rndc.out.ns2.$n)" = "$expected" ]
)
retry_quiet 10 _check_rndc_showzone_newly_added || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "deleting newly added zone ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone 'added.example in external' 2>&1 | sed 's/^/I:ns2 /'
_check_deleting_newly_added_zone() (
  $DIG $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.$n \
    && grep 'status: REFUSED' dig.out.ns2.$n >/dev/null \
    && ! grep '^a.added.example' dig.out.ns2.$n >/dev/null
)
retry_quiet 10 _check_deleting_newly_added_zone || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "attempting to add zone to internal view ($n)"
ret=0
$DIG +norec $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.added.example a >dig.out.ns2.pre.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.pre.$n >/dev/null || ret=1
$RNDCCMD 10.53.0.2 addzone 'added.example in internal { type primary; file "added.db"; };' 2>rndc.out.ns2.$n && ret=1
grep "permission denied" rndc.out.ns2.$n >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.added.example a >dig.out.ns2.int.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.int.$n >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.ext.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.ext.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "attempting to delete a policy zone ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone 'policy in internal' 2>rndc.out.ns2.$n >&1 && ret=1
grep 'cannot be deleted' rndc.out.ns2.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "adding new zone again to external view ($n)"
ret=0
$RNDCCMD 10.53.0.2 addzone 'added.example in external { type primary; file "added.db"; };' 2>&1 | sed 's/^/I:ns2 /'
_check_adding_new_zone_again_external() (
  $DIG +norec $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.added.example a >dig.out.ns2.int.$n \
    && grep 'status: NOERROR' dig.out.ns2.int.$n >/dev/null \
    && $DIG +norec $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.ext.$n \
    && grep 'status: NOERROR' dig.out.ns2.ext.$n >/dev/null \
    && grep '^a.added.example' dig.out.ns2.ext.$n >/dev/null
)
retry_quiet 10 _check_adding_new_zone_again_external || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "reconfiguring server with multiple views and new-zones-directory"
cp ns2/named3.conf ns2/named.conf
rndc_reconfig ns2 10.53.0.2

echo_i "checking new zone is still loaded after dir change ($n)"
ret=0
$DIG +norec $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.ext.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.ext.$n >/dev/null || ret=1
grep '^a.added.example' dig.out.ns2.ext.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "deleting newly added zone from external ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone 'added.example in external' 2>&1 | sed 's/^/I:ns2 /'
$DIG $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n >/dev/null || ret=1
grep '^a.added.example' dig.out.ns2.$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "adding new zone to directory view ($n)"
ret=0
$DIG +norec $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.added.example a >dig.out.ns2.intpre.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.intpre.$n >/dev/null || ret=1
$DIG +norec $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.extpre.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.extpre.$n >/dev/null || ret=1
$DIG +norec $DIGOPTS @10.53.0.5 -b 10.53.0.5 a.added.example a >dig.out.ns2.dirpre.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.dirpre.$n >/dev/null || ret=1
$RNDCCMD 10.53.0.2 addzone 'added.example in directory { type primary; file "added.db"; };' 2>&1 | sed 's/^/I:ns2 /'
$DIG +norec $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.added.example a >dig.out.ns2.int.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.int.$n >/dev/null || ret=1
$DIG +norec $DIGOPTS @10.53.0.4 -b 10.53.0.4 a.added.example a >dig.out.ns2.ext.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.ext.$n >/dev/null || ret=1
$DIG +norec $DIGOPTS @10.53.0.5 -b 10.53.0.5 a.added.example a >dig.out.ns2.dir.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.dir.$n >/dev/null || ret=1
grep '^a.added.example' dig.out.ns2.dir.$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if $FEATURETEST --with-lmdb; then
  echo_i "checking NZD file was created in new-zones-directory ($n)"
  expect=ns2/new-zones/directory.nzd
else
  echo_i "checking NZF file was created in new-zones-directory ($n)"
  expect=ns2/new-zones/directory.nzf
fi
$RNDCCMD 10.53.0.2 sync 'added.example IN directory' 2>&1 | sed 's/^/I:ns2 /'
sleep 2
[ -e "$expect" ] || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "deleting newly added zone from directory ($n)"
ret=0
$RNDCCMD 10.53.0.2 delzone 'added.example in directory' 2>&1 | sed 's/^/I:ns2 /'
$DIG $DIGOPTS @10.53.0.5 -b 10.53.0.5 a.added.example a >dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n >/dev/null || ret=1
grep '^a.added.example' dig.out.ns2.$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "ensure the configuration context is cleaned up correctly ($n)"
ret=0
rndc_reconfig ns2 10.53.0.2
$RNDCCMD 10.53.0.2 status >/dev/null 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "check delzone after reconfig failure ($n)"
ret=0
$RNDCCMD 10.53.0.3 addzone 'inlinesec.example. IN { type secondary; file "inlinesec.db"; masterfile-format text; primaries { test; }; };' >/dev/null 2>&1 || ret=1
cp ns3/named2.conf ns3/named.conf
rndc_reconfig ns3 10.53.0.3
$RNDCCMD 10.53.0.3 delzone inlinesec.example >/dev/null 2>&1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if ! $FEATURETEST --with-lmdb; then
  echo_i "check that addzone is fully reversed on failure (--with-lmdb=no) ($n)"
  ret=0
  $RNDCCMD 10.53.0.3 addzone "test1.baz" '{ type primary; file "e.db"; };' >/dev/null 2>&1 || ret=1
  $RNDCCMD 10.53.0.3 addzone "test2.baz" '{ type primary; file "dne.db"; };' >/dev/null 2>&1 && ret=1
  $RNDCCMD 10.53.0.3 addzone "test3.baz" '{ type primary; file "e.db"; };' >/dev/null 2>&1 || ret=1
  $RNDCCMD 10.53.0.3 delzone "test3.baz" >/dev/null 2>&1 || ret=1
  grep test2.baz ns3/_default.nzf >/dev/null && ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

_check_version_bind() (
  $DIG $DIGOPTS @10.53.0.3 version.bind txt ch >dig.out.test$n \
    && grep "status: NOERROR" dig.out.test$n >/dev/null
)

echo_i "check that named restarts with multiple added zones ($n)"
ret=0
$RNDCCMD 10.53.0.3 addzone "test4.baz" '{ type primary; file "e.db"; };' >/dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 addzone "test5.baz" '{ type primary; file "e.db"; };' >/dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 addzone '"test/.baz"' '{ type primary; check-names ignore; file "e.db"; };' >/dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 addzone '"test\".baz"' '{ type primary; check-names ignore; file "e.db"; };' >/dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 addzone '"test\\.baz"' '{ type primary; check-names ignore; file "e.db"; };' >/dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 addzone '"test\032.baz"' '{ type primary; check-names ignore; file "e.db"; };' >/dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 addzone '"test\010.baz"' '{ type primary; check-names ignore; file "e.db"; };' >/dev/null 2>&1 || ret=1
stop_server ns3
start_server --noclean --restart --port ${PORT} ns3 || ret=1
retry_quiet 10 _check_version_bind || ret=1
$DIG $DIGOPTS @10.53.0.3 SOA "test4.baz" >dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.1.test$n >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 SOA "test5.baz" >dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.2.test$n >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 SOA 'test/.baz' >dig.out.3.test$n || ret=1
grep "status: NOERROR" dig.out.3.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.3.test$n >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 SOA 'test\\.baz' >dig.out.4.test$n || ret=1
grep "status: NOERROR" dig.out.4.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.4.test$n >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 SOA 'test\032.baz' >dig.out.5.test$n || ret=1
grep "status: NOERROR" dig.out.5.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.5.test$n >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 SOA 'test\010.baz' >dig.out.6.test$n || ret=1
grep "status: NOERROR" dig.out.6.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.6.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
