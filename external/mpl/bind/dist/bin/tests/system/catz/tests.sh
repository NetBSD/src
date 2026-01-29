#!/bin/sh -x

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

# shellcheck source=conf.sh
. ../conf.sh

dig_with_opts() {
  "$DIG" -p "${PORT}" "$@"
}

rndccmd() (
  "$RNDC" -c ../_common/rndc.conf -p "${CONTROLPORT}" -s "$@"
)

_wait_for_message() (
  nextpartpeek "$1" >wait_for_message.$n
  grep -F "$2" wait_for_message.$n >/dev/null
)

wait_for_message() (
  retry_quiet 20 _wait_for_message "$@"
)

_wait_for_rcode() (
  rcode="$1"
  qtype="$2"
  ns="$3"
  qname="$4"
  file="$5"
  shift 5
  dig_with_opts "$ns" "$qtype" "$qname" "$@" >"$file" || return 1
  grep "status: $rcode" "$file" >/dev/null
)

wait_for_rcode() (
  retry_quiet 10 _wait_for_rcode "$@"
)

wait_for_soa() (
  wait_for_rcode NOERROR SOA "$@"
)

wait_for_a() (
  wait_for_rcode NOERROR A "$@"
)

wait_for_no_soa() {
  wait_for_rcode REFUSED SOA "$@"
}

_wait_for_zonefile() (
  # shellcheck disable=SC2234
  [ -f "$1" ]
)

wait_for_zonefile() (
  retry_quiet 10 _wait_for_zonefile "$@"
)

_wait_for_no_zonefile() (
  # shellcheck disable=SC2234
  [ ! -f "$1" ]
)

wait_for_no_zonefile() (
  retry_quiet 10 _wait_for_no_zonefile "$@"
)

status=0
n=0

##########################################################################
n=$((n + 1))
echo_i "checking that catalog-bad1.example (with no version) has failed to load ($n)"
ret=0
wait_for_message ns2/named.run "catz: zone 'catalog-bad1.example' has no 'version' record (partial match) and will not be processed" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that catalog-bad2.example (with unsupported version) has failed to load ($n)"
ret=0
wait_for_message ns2/named.run "catz: zone 'catalog-bad2.example' unsupported version '99'" \
  && wait_for_message ns2/named.run "catz: new catalog zone 'catalog-bad2.example' is broken and will not be processed" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that catalog-bad3.example (with two supported version records) has failed to load ($n)"
ret=0
wait_for_message ns2/named.run "catz: 'version' property TXT RRset contains more than one record, which is invalid" \
  && wait_for_message ns2/named.run "catz: invalid record in catalog zone - version.catalog-bad3.example IN TXT (failure) - ignoring" \
  && wait_for_message ns2/named.run "catz: zone 'catalog-bad3.example' version is not set" \
  && wait_for_message ns2/named.run "catz: new catalog zone 'catalog-bad3.example' is broken and will not be processed" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that catalog-bad4.example (with only spurious type A version record) has failed to load ($n)"
ret=0
wait_for_message ns2/named.run "catz: invalid record in catalog zone - version.catalog-bad4.example IN A (failure) - ignoring" \
  && wait_for_message ns2/named.run "catz: zone 'catalog-bad4.example' version is not set" \
  && wait_for_message ns2/named.run "catz: new catalog zone 'catalog-bad4.example' is broken and will not be processed" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that catalog-bad5.example (non-IN class) has failed to load ($n)"
ret=0
wait_for_message ns2/named.run "'catalog-zones' option is only supported for views with class IN" \
  && wait_for_message ns2/named.run "all zones loaded" || ret=1
grep -F "catz: dns_catz_zone_add catalog-bad5.example" ns2/named.run && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

##########################################################################
echo_i "Testing adding/removing of domain in catalog zone"
n=$((n + 1))
echo_i "checking that dom1.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain dom1.example. to primary via RNDC ($n)"
ret=0
# enough initial content for IXFR response when TXT record is added below
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom1.example.db
echo "@ 3600 IN NS invalid." >>ns1/dom1.example.db
echo "foo 3600 IN TXT some content here" >>ns1/dom1.example.db
echo "bar 3600 IN TXT some content here" >>ns1/dom1.example.db
echo "xxx 3600 IN TXT some content here" >>ns1/dom1.example.db
echo "yyy 3600 IN TXT some content here" >>ns1/dom1.example.db
rndccmd 10.53.0.1 addzone dom1.example. in default '{ type primary; file "dom1.example.db"; allow-update { any; }; notify explicit; also-notify { 10.53.0.2; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom1.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain dom1.example. to catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add e721433b6160b450260d4f54b3ec8bab30cb3b83.zones.catalog1.example. 3600 IN PTR dom1.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom1.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom1.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom1.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that zone-directory is populated ($n)"
ret=0
wait_for_zonefile "ns2/zonedir/__catz__default_catalog1.example_dom1.example.db" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "update dom1.example. ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update add dom1.example 0 IN TXT added record
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "wait for secondary to be updated ($n)"
ret=0
wait_for_txt() {
  dig_with_opts @10.53.0.2 TXT dom1.example. >dig.out.test$n || return 1
  grep "ANSWER: 1," dig.out.test$n >/dev/null || return 1
  grep "status: NOERROR" dig.out.test$n >/dev/null || return 1
  grep "IN.TXT." dig.out.test$n >/dev/null || return 1
}
retry_quiet 10 wait_for_txt || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that journal was created for cleanup test ($n)"
ret=0
test -f ns2/zonedir/__catz__default_catalog1.example_dom1.example.db.jnl || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "update catalog zone serial ($n)"
ret=0
# default minimum update rate is once / 5 seconds
sleep 5
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add catalog1.example 3600 SOA . . 20 86400 3600 86400 3600
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "wait for catalog zone to transfer ($n)"
ret=0
wait_for_soa_equal_20() {
  dig_with_opts @10.53.0.2 SOA catalog1.example. >dig.out.test$n || return 1
  grep "ANSWER: 1," dig.out.test$n >/dev/null || return 1
  grep "status: NOERROR" dig.out.test$n >/dev/null || return 1
  grep 'IN.SOA.\. \. 20 ' dig.out.test$n >/dev/null || return 1
}
retry_quiet 10 wait_for_soa_equal_20 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "update dom1.example. again ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update add foo.dom1.example 0 IN TXT added record
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "wait for secondary to be updated again ($n)"
ret=0
wait_for_txt() {
  dig_with_opts @10.53.0.2 TXT foo.dom1.example. >dig.out.test$n || return 1
  grep "ANSWER: 2," dig.out.test$n >/dev/null || return 1
  grep "status: NOERROR" dig.out.test$n >/dev/null || return 1
  grep "IN.TXT." dig.out.test$n >/dev/null || return 1
}
retry_quiet 10 wait_for_txt || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "removing domain dom1.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update delete e721433b6160b450260d4f54b3ec8bab30cb3b83.zones.catalog1.example
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: catz_delzone_cb: zone 'dom1.example' deleted" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom1.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that zone-directory is emptied ($n)"
ret=0
wait_for_no_zonefile "ns2/zonedir/__catz__default_catalog1.example_dom1.example.db" || ret=1
wait_for_no_zonefile "ns2/zonedir/__catz__default_catalog1.example_dom1.example.db.jnl" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

##########################################################################
echo_i "Testing various simple operations on domains, including using multiple catalog zones and garbage in zone"
n=$((n + 1))
echo_i "adding domain dom2.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom2.example.db
echo "@ IN NS invalid." >>ns1/dom2.example.db
echo "@ IN A 192.0.2.1" >>ns1/dom2.example.db
rndccmd 10.53.0.1 addzone dom2.example. in default '{type primary; file "dom2.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding domain dom2.example. to primary ns3 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns3/dom2.example.db
echo "@ IN NS invalid." >>ns3/dom2.example.db
echo "@ IN A 192.0.2.2" >>ns3/dom2.example.db
rndccmd 10.53.0.3 addzone dom2.example. '{type primary; file "dom2.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding domain dom4.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom4.example.db
echo "@ IN NS invalid." >>ns1/dom4.example.db
rndccmd 10.53.0.1 addzone dom4.example. in default '{type primary; file "dom4.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding domains dom2.example, dom3.example. and some garbage to catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR dom2.example.
    update add coo.636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN TXT "catalog2.example."
    update add b901f492f3ebf6c1e5b597e51766f02f0479eb03.zones.catalog1.example. 3600 IN PTR dom3.example.
    update add e721433b6160b450260d4f54b3ec8bab30cb3b83.zones.catalog1.example. 3600 IN NS foo.bar.
    update add trash.catalog1.example. 3600 IN A 1.2.3.4
    update add trash2.foo.catalog1.example. 3600 IN A 1.2.3.4
    update add trash3.zones.catalog1.example. 3600 IN NS a.dom2.example.
    update add foobarbaz.b901f492f3ebf6c1e5b597e51766f02f0479eb03.zones.catalog1.example. 3600 IN PTR dom3.example.
    update add blahblah.636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR dom2.example.
    update add foobarbaz.b901f492f3ebf6c1e5b597e51766f02f0479eb03.zones.catalog1.example. 3600 IN APL 1:1.2.3.4/30
    update add blahblah.636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN TXT "blah blah"
    update add version.catalog1.example. 3600 IN A 1.2.3.4
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding domain dom4.example. to catalog2 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add de26b88d855397a03f77ff1162fd055d8b419584.zones.catalog2.example. 3600 IN PTR dom4.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: updating catalog zone 'catalog2.example' with serial 2670950425" \
  && wait_for_message ns2/named.run "catz: adding zone 'dom2.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "catz: adding zone 'dom3.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "catz: adding zone 'dom4.example' from catalog 'catalog2.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom4.example/IN/default' from 10.53.0.1#${EXTRAPORT1}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom4.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom4.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom3.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding a domain dom3.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom3.example.db
echo "@ IN NS invalid." >>ns1/dom3.example.db
rndccmd 10.53.0.1 addzone dom3.example. in default '{type primary; file "dom3.example.db"; also-notify { 10.53.0.2; }; notify explicit; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom3.example. is served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "transfer of 'dom2.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" \
  && wait_for_message ns2/named.run "transfer of 'dom3.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom3.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

# The member zone's PTR RRset must have only one record in it.
# Check that adding a second record to the RRset is caught and such a
# catalog zone is not processed.
n=$((n + 1))
echo_i "adding domain dom4-reused-label.example. to catalog2 zone, reusing a label ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add de26b88d855397a03f77ff1162fd055d8b419584.zones.catalog2.example. 3600 IN PTR dom4-reused-label.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up, and checking that the reused label has been caught ($n)"
ret=0
wait_for_message ns2/named.run "de26b88d855397a03f77ff1162fd055d8b419584.zones.catalog2.example IN PTR (failure)" \
  && wait_for_message ns2/named.run "catz: new catalog zone 'catalog2.example' is broken and will not be processed" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "deleting domain dom4-reused-label.example. from catalog2 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete de26b88d855397a03f77ff1162fd055d8b419584.zones.catalog2.example. 3600 IN PTR dom4-reused-label.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

# Test zone associated state reset.
n=$((n + 1))
echo_i "renaming the label of domain dom4.example. in catalog2 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete de26b88d855397a03f77ff1162fd055d8b419584.zones.catalog2.example. 3600 IN PTR dom4.example.
    update add dom4-renamed-label.zones.catalog2.example. 3600 IN PTR dom4.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up, and checking that the zone has been reset ($n)"
ret=0
wait_for_message ns2/named.run "catz: zone 'dom4.example' unique label has changed, reset state" \
  && wait_for_message ns2/named.run "catz: deleting zone 'dom4.example' from catalog 'catalog2.example' - success" \
  && wait_for_message ns2/named.run "catz: adding zone 'dom4.example' from catalog 'catalog2.example' - success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding domain dom2.example. to catalog2 zone to test change of ownership ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add dom2-without-coo.zones.catalog2.example. 3600 IN PTR dom2.example.
    update add primaries.dom2-without-coo.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom2.example' from catalog 'catalog2.example'" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that unpermitted change of ownership did not happen ($n)"
ret=0
wait_for_message ns2/named.run "catz_addmodzone_cb: zone 'dom2.example' will not be added because another catalog zone already contains an entry with that zone" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom2.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom2.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding change of ownership permission record for dom2.example. into catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add coo.636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR catalog2.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: updating catalog zone 'catalog1.example'" \
  && wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "updating catalog2 zone to initiate a zone transfer ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete dom2-without-coo.zones.catalog2.example. 3600 IN PTR dom2.example.
    update delete primaries.dom2-without-coo.zones.catalog2.example. 3600 IN A 10.53.0.3
    update add dom2-with-coo.zones.catalog2.example. 3600 IN PTR dom2.example.
    update add primaries.dom2-with-coo.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up and checking that the change of ownership was successful ($n)"
ret=0
wait_for_message ns2/named.run "catz: zone 'dom2.example' change of ownership from 'catalog1.example' to 'catalog2.example'" \
  && wait_for_message ns2/named.run "catz: deleting zone 'dom2.example' from catalog 'catalog1.example' - success" \
  && wait_for_message ns2/named.run "catz: adding zone 'dom2.example' from catalog 'catalog2.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom2.example/IN/default' from 10.53.0.3#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom2.example. is served by secondary and that it's now the one from ns3 ($n)"
ret=0
wait_for_a @10.53.0.2 dom2.example. dig.out.test$n || ret=1
grep "192.0.2.2" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "removing dom2.example. and its change of ownership permission record from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete 636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR dom2.example.
    update delete coo.636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR catalog2.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: iteration finished" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding change of ownership permission record for dom2.example. into catalog2 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add coo.dom2-with-coo.zones.catalog2.example. 3600 IN PTR catalog1.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: iteration finished" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding back dom2.example. into catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR dom2.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the change of ownership did not happen because version '1' catalog2 zone does not support the 'coo' property ($n)"
ret=0
wait_for_message ns2/named.run "catz_addmodzone_cb: zone 'dom2.example' will not be added because another catalog zone already contains an entry with that zone" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom2.example. is still served by secondary and that it's still the one from ns3 ($n)"
ret=0
wait_for_a @10.53.0.2 dom2.example. dig.out.test$n || ret=1
grep "192.0.2.2" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

# GL #3060
n=$((n + 1))
echo_i "reconfiguring secondary - checking if catz survives a certain class of failed reconfiguration attempts ($n)"
ret=0
cp ns2/named2.conf ns2/named.conf
$RNDC -c ../_common/rndc.conf -s 10.53.0.2 -p "${CONTROLPORT}" reconfig >/dev/null 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking again that dom3.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "reconfiguring secondary - reverting the bad configuration ($n)"
ret=0
cp ns2/named1.conf ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

# GL #3911
n=$((n + 1))
echo_i "reconfiguring secondary - checking if catz survives another type of failed reconfiguration attempts ($n)"
ret=0
cp ns2/named3.conf ns2/named.conf
$RNDC -c ../_common/rndc.conf -s 10.53.0.2 -p "${CONTROLPORT}" reconfig >/dev/null 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# catalog zone update can be deferred
sleep 2

n=$((n + 1))
echo_i "checking again that dom3.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# GL #4733
n=$((n + 1))
echo_i "reconfiguring secondary - checking if catz member zones are reconfigured ($n)"
ret=0
cp ns2/named4.conf ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom3.example. is refused by secondary because of an activated allow-query ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "reconfiguring secondary - reverting the bad configuration ($n)"
ret=0
cp ns2/named1.conf ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding a domain dom-existing.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom-existing.example.db
echo "@ IN NS invalid." >>ns1/dom-existing.example.db
echo "@ IN A 192.0.2.1" >>ns1/dom-existing.example.db
rndccmd 10.53.0.1 addzone dom-existing.example. in default '{type primary; file "dom-existing.example.db"; also-notify { 10.53.0.2; }; notify explicit; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom-existing.example. is served by primary ($n)"
ret=0
wait_for_a @10.53.0.1 dom-existing.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding domain dom-existing.example. to catalog1 zone to test that existing zones don't get overwritten ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add dom-existing.zones.catalog1.example. 3600 IN PTR dom-existing.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom-existing.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "catz_addmodzone_cb: zone 'dom-existing.example' will not be added because it is an explicitly configured zone" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom-existing.example. is served by secondary and that it's not the one from the primary ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom-existing.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding a domain dom-existing-forward.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom-existing-forward.example.db
echo "@ IN NS invalid." >>ns1/dom-existing-forward.example.db
echo "@ IN A 192.0.2.1" >>ns1/dom-existing-forward.example.db
rndccmd 10.53.0.1 addzone dom-existing-forward.example. in default '{type primary; file "dom-existing-forward.example.db"; also-notify { 10.53.0.2; }; notify explicit; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom-existing-forward.example. is served by primary ($n)"
ret=0
wait_for_a @10.53.0.1 dom-existing-forward.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding domain dom-existing-forward.example. to catalog1 zone to test that existing forward zones don't get overwritten ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add dom-existing-forward.zones.catalog1.example. 3600 IN PTR dom-existing-forward.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom-existing-forward.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "catz_addmodzone_cb: zone 'dom-existing-forward.example' will not be processed because of the explicitly configured forwarding for that zone" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom-existing-forward.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom-existing-forward.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding a domain dom-existing-forward-off.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom-existing-forward-off.example.db
echo "@ IN NS invalid." >>ns1/dom-existing-forward-off.example.db
echo "@ IN A 192.0.2.1" >>ns1/dom-existing-forward-off.example.db
rndccmd 10.53.0.1 addzone dom-existing-forward-off.example. in default '{type primary; file "dom-existing-forward-off.example.db"; also-notify { 10.53.0.2; }; notify explicit; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom-existing-forward-off.example. is served by primary ($n)"
ret=0
wait_for_a @10.53.0.1 dom-existing-forward-off.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding domain dom-existing-forward-off.example. to catalog1 zone to test that a zone with turned off forwarding can be used in a catalog zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add dom-existing-forward-off.zones.catalog1.example. 3600 IN PTR dom-existing-forward-off.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom-existing-forward-off.example' from catalog 'catalog1.example'" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom-existing-forward-off.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom-existing-forward-off.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "removing all records from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete 636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR dom2.example.
    update delete coo.636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN TXT "catalog2.example."
    update delete b901f492f3ebf6c1e5b597e51766f02f0479eb03.zones.catalog1.example. 3600 IN PTR dom3.example.
    update delete e721433b6160b450260d4f54b3ec8bab30cb3b83.zones.catalog1.example. 3600 IN NS foo.bar.
    update delete trash.catalog1.example. 3600 IN A 1.2.3.4
    update delete trash2.foo.catalog1.example. 3600 IN A 1.2.3.4
    update delete trash3.zones.catalog1.example. 3600 IN NS a.dom2.example.
    update delete foobarbaz.b901f492f3ebf6c1e5b597e51766f02f0479eb03.zones.catalog1.example. 3600 IN PTR dom3.example.
    update delete blahblah.636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR dom2.example.
    update delete foobarbaz.b901f492f3ebf6c1e5b597e51766f02f0479eb03.zones.catalog1.example. 3600 IN APL 1:1.2.3.4/30
    update delete blahblah.636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN TXT "blah blah"
    update delete version.catalog1.example. 3600 IN A 1.2.3.4
    update delete dom-existing.zones.catalog1.example. 3600 IN PTR dom-existing.example.
    update delete dom-existing-forward.zones.catalog1.example. 3600 IN PTR dom-existing-forward.example.
    update delete dom-existing-forward-off.zones.catalog1.example. 3600 IN PTR dom-existing-forward.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "removing all records from catalog2 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete dom2-with-coo.zones.catalog2.example. 3600 IN PTR dom2.example.
    update delete primaries.dom2-with-coo.zones.catalog2.example. 3600 IN A 10.53.0.3
    update delete coo.dom2-with-coo.zones.catalog2.example. 3600 IN PTR catalog1.example.
    update delete dom4-renamed-label.zones.catalog2.example. 3600 IN PTR dom4.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing primaries suboption and random labels"
n=$((n + 1))
echo_i "adding dom5.example. with a valid primaries suboption (IP without TSIG) and a random label ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add somerandomlabel.zones.catalog1.example. 3600 IN PTR dom5.example.
    update add primaries.ext.somerandomlabel.zones.catalog1.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom5.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom5.example/IN/default' from 10.53.0.3#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom5.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom5.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "removing dom5.example. ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete somerandomlabel.zones.catalog1.example. 3600 IN PTR dom5.example.
    update delete primaries.somerandomlabel.zones.catalog1.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: catz_delzone_cb: zone 'dom5.example' deleted" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom5.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom5.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing primaries global option"
n=$((n + 1))
echo_i "adding dom6.example. and a valid global primaries option (IP without TSIG) ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add primaries.ext.catalog1.example. 3600 IN A 10.53.0.3
    update add primaries.ext.catalog1.example. 3600 IN AAAA fd92:7065:b8e:ffff::3
    update add 4346f565b4d63ddb99e5d2497ff22d04e878e8f8.zones.catalog1.example. 3600 IN PTR dom6.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom6.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom6.example/IN/default' from " >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom6.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom6.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "removing dom6.example. ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete primaries.ext.catalog1.example. 3600 IN A 10.53.0.3
    update delete primaries.ext.catalog1.example. 3600 IN AAAA  fd92:7065:b8e:ffff::3
    update delete 4346f565b4d63ddb99e5d2497ff22d04e878e8f8.zones.catalog1.example. 3600 IN PTR dom6.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: catz_delzone_cb: zone 'dom6.example' deleted" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom6.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom6.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding dom6.example. and an invalid global primaries option (TSIG without IP) ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add label1.primaries.ext.catalog1.example. 3600 IN TXT "tsig_key"
    update add 4346f565b4d63ddb99e5d2497ff22d04e878e8f8.zones.catalog1.example. 3600 IN PTR dom6.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom6.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "error \"failure\" while trying to generate config for zone 'dom6.example'" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "removing dom6.example. ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete label1.primaries.ext.catalog1.example. 3600 IN TXT "tsig_key"
    update delete 4346f565b4d63ddb99e5d2497ff22d04e878e8f8.zones.catalog1.example. 3600 IN PTR dom6.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: deleting zone 'dom6.example' from catalog 'catalog1.example' - success" >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
n=$((n + 1))
echo_i "Checking that a missing zone directory forces in-memory ($n)"
ret=0
grep "'nonexistent' not found; zone files will not be saved" ns2/named.run >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing allow-query and allow-transfer ACLs"
n=$((n + 1))
echo_i "adding domains dom7.example. and dom8.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom7.example.db
echo "@ IN NS invalid." >>ns1/dom7.example.db
rndccmd 10.53.0.1 addzone dom7.example. in default '{type primary; file "dom7.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom8.example.db
echo "@ IN NS invalid." >>ns1/dom8.example.db
rndccmd 10.53.0.1 addzone dom8.example. in default '{type primary; file "dom8.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom7.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom7.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding domain dom7.example. to catalog1 zone with an allow-query statement ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 78833ec3c0059fd4540fee81c7eaddce088e7cd7.zones.catalog1.example. 3600 IN PTR dom7.example.
    update add allow-query.ext.78833ec3c0059fd4540fee81c7eaddce088e7cd7.zones.catalog1.example. 3600 IN APL 1:10.53.0.1/32 !1:10.53.0.0/30 1:0.0.0.0/0
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom7.example' from catalog 'catalog1.example'" >/dev/null \
  && wait_for_message ns2/named.run "transfer of 'dom7.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom7.example. is accessible from 10.53.0.1 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom7.example. dig.out.test$n -b 10.53.0.1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom7.example. is not accessible from 10.53.0.2 ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom7.example. dig.out.test$n -b 10.53.0.2 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom7.example. is accessible from 10.53.0.5 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom7.example. dig.out.test$n -b 10.53.0.5 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null
n=$((n + 1))
echo_i "adding dom8.example. domain and global allow-query and allow-transfer ACLs ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add cba95222e308baba42417be6021026fdf20827b6.zones.catalog1.example. 3600 IN PTR dom8.example
    update add allow-query.ext.catalog1.example. 3600 IN APL 1:10.53.0.1/32
    update add allow-transfer.ext.catalog1.example. 3600 IN APL 1:10.53.0.2/32
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" \
  && wait_for_message ns2/named.run "transfer of 'dom8.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom8.example. is accessible from 10.53.0.1 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom8.example. dig.out.test$n -b 10.53.0.1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom8.example. is not accessible from 10.53.0.2 ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom8.example. dig.out.test$n -b 10.53.0.2 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom8.example. is not AXFR accessible from 10.53.0.1 ($n)"
ret=0
dig_with_opts @10.53.0.2 axfr dom8.example. -b 10.53.0.1 >dig.out.test$n || ret=1
grep "Transfer failed." dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom8.example. is AXFR accessible from 10.53.0.2 ($n)"
ret=0
dig_with_opts @10.53.0.2 axfr dom8.example. -b 10.53.0.2 >dig.out.test$n || ret=1
grep -v "Transfer failed." dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null
n=$((n + 1))
echo_i "deleting global allow-query and allow-domain ACLs ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete allow-query.ext.catalog1.example. 3600 IN APL 1:10.53.0.1/32
    update delete allow-transfer.ext.catalog1.example. 3600 IN APL 1:10.53.0.2/32
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom8.example. is accessible from 10.53.0.1 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom8.example. dig.out.test$n -b 10.53.0.1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom8.example. is accessible from 10.53.0.2 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom8.example. dig.out.test$n -b 10.53.0.2 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom8.example. is AXFR accessible from 10.53.0.1 ($n)"
ret=0
dig_with_opts @10.53.0.2 axfr dom8.example. -b 10.53.0.1 >dig.out.test$n || ret=1
grep -v "Transfer failed." dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom8.example. is AXFR accessible from 10.53.0.2 ($n)"
ret=0
dig_with_opts @10.53.0.2 axfr dom8.example. -b 10.53.0.2 >dig.out.test$n || ret=1
grep -v "Transfer failed." dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing TSIG keys for primaries set per-domain"
n=$((n + 1))
echo_i "adding a domain dom9.example. to primary via RNDC, with transfers allowed only with TSIG key ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom9.example.db
echo "@ IN NS invalid." >>ns1/dom9.example.db
rndccmd 10.53.0.1 addzone dom9.example. in default '{type primary; file "dom9.example.db"; allow-transfer { key tsig_key; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom9.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding domain dom9.example. to catalog1 zone with a valid primaries suboption (IP with TSIG) ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN PTR dom9.example.
    update add label1.primaries.ext.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN A 10.53.0.1
    update add label1.primaries.ext.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "tsig_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom9.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom9.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom9.example. is accessible on secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "change TSIG key name on primary ($n)"
ret=0
rndccmd 10.53.0.1 modzone dom9.example. in default '{type primary; notify yes; file "dom9.example.db"; allow-transfer { key next_key; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "update TSIG key name in catalog zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update del label1.primaries.ext.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "tsig_key"
    update add label1.primaries.ext.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "next_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: modifying zone 'dom9.example' from catalog 'catalog1.example'" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "update zone contents and reload ($n)"
ret=0
echo "@ 3600 IN SOA . . 2 3600 3600 3600 3600" >ns1/dom9.example.db
echo "@ IN NS ns2" >>ns1/dom9.example.db
echo "ns2 IN A 10.53.0.2" >>ns1/dom9.example.db
rndccmd 10.53.0.1 reload dom9.example. || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "wait for primary to update zone ($n)"
ret=0
wait_for_a @10.53.0.1 ns2.dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "wait for secondary to update zone ($n)"
ret=0
wait_for_a @10.53.0.2 ns2.dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "deleting domain dom9.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN PTR dom9.example.
    update delete label1.primaries.ext.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN A 10.53.0.1
    update delete label1.primaries.ext.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "next_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: deleting zone 'dom9.example' from catalog 'catalog1.example' - success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom9.example. is no longer accessible on secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding domain dom9.example. to catalog1 zone with an invalid primaries suboption (TSIG without IP) ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN PTR dom9.example.
    update add label1.primaries.ext.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "tsig_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom9.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "error \"failure\" while trying to generate config for zone 'dom9.example'" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "deleting domain dom9.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN PTR dom9.example.
    update delete label1.primaries.ext.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "tsig_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: deleting zone 'dom9.example' from catalog 'catalog1.example'" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing catalog entries that can't be represented as filenames"
# note: we need 4 backslashes in the shell to get 2 backslashes in DNS
# presentation format, which is 1 backslash on the wire.
for special in \
  this.is.a.very.very.long.long.long.domain.that.will.cause.catalog.zones.to.generate.hash.instead.of.using.regular.filename.dom10.example \
  this.zone/domain.has.a.slash.dom10.example \
  this.zone\\\\domain.has.backslash.dom10.example \
  this.zone:domain.has.a.colon.dom.10.example; do
  # hashes below are generated by:
  # python ${TOP}/contrib/scripts/catzhash.py "${special}"

  case "$special" in
    this.is.a.very.very.long.long.long.domain.that.will.cause.catalog.zones.to.generate.hash.instead.of.using.regular.filename.dom10.example)
      hash=825f48b1ce1b4cf5a041d20255a0c8e98d114858
      db=__catz__a35e0a044ff9f37436068e1e83e9b99fb9da51b0fe7b699bdb404f7755d68276.db
      ;;
    this.zone/domain.has.a.slash.dom10.example)
      hash=e64cc64c99bf52d0a77fb16dd7ed57cf925a36aa
      db=__catz__765197c8050c794f4ec5bbf5dbdf64d0551459c08a91a4217768fcd16cd3b7ce.db
      ;;
    this.zone\\\\domain.has.backslash.dom10.example)
      hash=91e27e02153d38cf656a9b376d7747fbcd19f985
      db=__catz__0f2f3beaf2ef70e0086063ae28a69444cdf3847cb85e668bfe52c89f7f756b29.db
      ;;
    this.zone:domain.has.a.colon.dom.10.example)
      hash=8b7238bf4c34045834c573ba4116557ebb24d33c
      db=__catz__ba75ab860533508a62b0937c5c6b8537e4186e4d5e7685161287260d07418251.db
      ;;
  esac

  n=$((n + 1))
  echo_i "checking that ${special}. is not served by primary ($n)"
  ret=0
  wait_for_no_soa @10.53.0.1 "${special}" dig.out.test$n || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "Adding a domain ${special}. to primary via RNDC ($n)"
  ret=0
  echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom10.example.db
  echo "@ IN NS invalid." >>ns1/dom10.example.db
  rndccmd 10.53.0.1 addzone '"'"${special}"'"' in default '{type primary; file "dom10.example.db";};' || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking that ${special}. is now served by primary ($n)"
  ret=0
  wait_for_soa @10.53.0.1 "${special}." dig.out.test$n || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  nextpart ns2/named.run >/dev/null

  n=$((n + 1))
  echo_i "Adding domain ${special}. to catalog1 zone ($n)"
  ret=0
  $NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
      server 10.53.0.1 ${PORT}
      update add ${hash}.zones.catalog1.example 3600 IN PTR ${special}.
      send
END
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "waiting for secondary to sync up ($n)"
  ret=0
  wait_for_message ns2/named.run "catz: adding zone '$special' from catalog 'catalog1.example'" \
    && wait_for_message ns2/named.run "transfer of '$special/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking that ${special}. is served by secondary ($n)"
  ret=0
  wait_for_soa @10.53.0.2 "${special}." dig.out.test$n || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking that zone-directory is populated with a hashed filename ($n)"
  ret=0
  wait_for_zonefile "ns2/zonedir/$db" || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "removing domain ${special}. from catalog1 zone ($n)"
  ret=0
  $NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
      server 10.53.0.1 ${PORT}
      update delete ${hash}.zones.catalog1.example
      send
END
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "waiting for secondary to sync up ($n)"
  ret=0
  wait_for_message ns2/named.run "catz: catz_delzone_cb: zone '${special}' deleted" || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking that ${special}. is not served by secondary ($n)"
  ret=0
  wait_for_no_soa @10.53.0.2 "${special}." dig.out.test$n || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking that zone-directory is emptied ($n)"
  ret=0
  wait_for_no_zonefile "ns2/zonedir/$db" || ret=1
  wait_for_no_zonefile "ns2/zonedir/$db.jnl" || ret=1
  if [ $ret -ne 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
done

##########################################################################
echo_i "Testing adding a domain and a subdomain of it"
n=$((n + 1))
echo_i "checking that dom11.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain dom11.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom11.example.db
echo "@ IN NS invalid." >>ns1/dom11.example.db
rndccmd 10.53.0.1 addzone dom11.example. in default '{type primary; file "dom11.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom11.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain dom11.example. to catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 0580d70e769c86c8b951a488d8b776627f427d7a.zones.catalog1.example. 3600 IN PTR dom11.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom11.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom11.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom11.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that subdomain.of.dom11.example. is not served by primary ($n)"
ret=0
wait_for_rcode NXDOMAIN SOA @10.53.0.1 subdomain.of.dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain subdomain.of.dom11.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/subdomain.of.dom11.example.db
echo "@ IN NS invalid." >>ns1/subdomain.of.dom11.example.db
rndccmd 10.53.0.1 addzone subdomain.of.dom11.example. in default '{type primary; file "subdomain.of.dom11.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that subdomain.of.dom11.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 subdomain.of.dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain subdomain.of.dom11.example. to catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 25557e0bdd10cb3710199bb421b776df160f241e.zones.catalog1.example. 3600 IN PTR subdomain.of.dom11.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'subdomain.of.dom11.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'subdomain.of.dom11.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that subdomain.of.dom11.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 subdomain.of.dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "removing domain dom11.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update delete 0580d70e769c86c8b951a488d8b776627f427d7a.zones.catalog1.example
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: catz_delzone_cb: zone 'dom11.example' deleted" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom11.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that subdomain.of.dom11.example. is still served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 subdomain.of.dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "removing domain subdomain.of.dom11.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update delete 25557e0bdd10cb3710199bb421b776df160f241e.zones.catalog1.example
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: catz_delzone_cb: zone 'subdomain.of.dom11.example' deleted" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that subdomain.of.dom11.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 subdomain.of.d11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing adding a catalog zone at runtime with rndc reconfig"
n=$((n + 1))
echo_i "checking that dom12.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain dom12.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom12.example.db
echo "@ IN NS invalid." >>ns1/dom12.example.db
rndccmd 10.53.0.1 addzone dom12.example. in default '{type primary; file "dom12.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom12.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain dom12.example. to catalog4 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 871d51e5433543c0f6fb263c40f359fbc152c8ae.zones.catalog4.example. 3600 IN PTR dom12.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom12.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "reconfiguring secondary - adding catalog4 catalog zone ($n)"
ret=0
cp ns2/named5.conf ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom12.example' from catalog 'catalog4.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom12.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom7.example. is still served by secondary after reconfiguration ($n)"
ret=0
wait_for_soa @10.53.0.2 dom7.example. dig.out.test$n -b 10.53.0.1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "checking that dom12.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "reconfiguring secondary - removing catalog4 catalog zone, adding non-existent catalog5 catalog zone ($n)"
ret=0
cp ns2/named6.conf ns2/named.conf
$RNDC -c ../_common/rndc.conf -s 10.53.0.2 -p "${CONTROLPORT}" reconfig >/dev/null 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "reconfiguring secondary - removing non-existent catalog5 catalog zone ($n)"
ret=0
cp ns2/named1.conf ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom12.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "removing domain dom12.example. from catalog4 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete 871d51e5433543c0f6fb263c40f359fbc152c8ae.zones.catalog4.example. 3600 IN PTR dom12.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing having a zone in two different catalogs"
n=$((n + 1))
echo_i "checking that dom13.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom13.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain dom13.example. to primary ns1 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom13.example.db
echo "@ IN NS invalid." >>ns1/dom13.example.db
echo "@ IN A 192.0.2.1" >>ns1/dom13.example.db
rndccmd 10.53.0.1 addzone dom13.example. in default '{type primary; file "dom13.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom13.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom13.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain dom13.example. to primary ns3 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns3/dom13.example.db
echo "@ IN NS invalid." >>ns3/dom13.example.db
echo "@ IN A 192.0.2.2" >>ns3/dom13.example.db
rndccmd 10.53.0.3 addzone dom13.example. '{type primary; file "dom13.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom13.example. is now served by primary ns3 ($n)"
ret=0
wait_for_soa @10.53.0.3 dom13.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain dom13.example. to catalog1 zone with ns1 as primary ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog1.example. 3600 IN PTR dom13.example.
    update add primaries.ext.8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog1.example. 3600 IN A 10.53.0.1
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom13.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom13.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "checking that dom13.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom13.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding domain dom13.example. to catalog2 zone with ns3 as primary ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add 8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog2.example. 3600 IN PTR dom13.example.
    update add primaries.8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom13.example. is served by secondary and that it's still the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom13.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Deleting domain dom13.example. from catalog2 ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete 8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog2.example. 3600 IN PTR dom13.example.
    update delete primaries.8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom13.example. is served by secondary and that it's still the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom13.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Deleting domain dom13.example. from catalog1 ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete 8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog1.example. 3600 IN PTR dom13.example.
    update delete primaries.ext.8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog1.example. 3600 IN A 10.53.0.2
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom13.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom13.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing recreation of a manually deleted zone after a reload"
n=$((n + 1))
echo_i "checking that dom16.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom16.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain dom16.example. to primary ns1 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom16.example.db
echo "@ IN NS invalid." >>ns1/dom16.example.db
echo "@ IN A 192.0.2.1" >>ns1/dom16.example.db
rndccmd 10.53.0.1 addzone dom16.example. in default '{type primary; file "dom16.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom16.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom16.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain dom16.example. to catalog1 zone with ns1 as primary ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add efe725d0cf430ffb113b9bcf59266f066a21216b.zones.catalog1.example. 3600 IN PTR dom16.example.
    update add masters.efe725d0cf430ffb113b9bcf59266f066a21216b.zones.catalog1.example. 3600 IN A 10.53.0.1
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom16.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom16.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "checking that dom16.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom16.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

echo_i "Deleting dom16.example. from secondary ns2 via RNDC ($n)"
ret=0
rndccmd 10.53.0.2 delzone dom16.example. in default >/dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom16.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom16.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

echo_i "Reloading secondary ns2 via RNDC ($n)"
ret=0
rndccmd 10.53.0.2 reload >/dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: zone 'dom16.example' was expected to exist but can not be found, will be restored" || ret=1
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
wait_for_message ns2/named.run "catz: catalog1.example: reload done: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom16.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom16.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom8.example. was not accidentally deleted during the configuration ($n)"
ret=0
_wait_for_message ns2/named.run "catz: zone 'dom8.example' was expected to exist but can not be found, will be restored" && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Deleting domain dom16.example. from catalog1 ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete efe725d0cf430ffb113b9bcf59266f066a21216b.zones.catalog1.example. 3600 IN PTR dom16.example.
    update delete masters.efe725d0cf430ffb113b9bcf59266f066a21216b.zones.catalog1.example. 3600 IN A 10.53.0.1
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom16.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom16.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing having a regular zone and a zone in catalog zone of the same name"
n=$((n + 1))
echo_i "checking that dom14.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom14.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain dom14.example. to primary ns1 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom14.example.db
echo "@ IN NS invalid." >>ns1/dom14.example.db
echo "@ IN A 192.0.2.1" >>ns1/dom14.example.db
rndccmd 10.53.0.1 addzone dom14.example. in default '{type primary; file "dom14.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom14.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom14.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain dom14.example. to primary ns3 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns3/dom14.example.db
echo "@ IN NS invalid." >>ns3/dom14.example.db
echo "@ IN A 192.0.2.2" >>ns3/dom14.example.db
rndccmd 10.53.0.3 addzone dom14.example. '{type primary; file "dom14.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom14.example. is now served by primary ns3 ($n)"
ret=0
wait_for_soa @10.53.0.3 dom14.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain dom14.example. with rndc with ns1 as primary ($n)"
ret=0
rndccmd 10.53.0.2 addzone dom14.example. in default '{type secondary; primaries {10.53.0.1;};};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "transfer of 'dom14.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "checking that dom14.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom14.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding domain dom14.example. to catalog2 zone with ns3 as primary ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add 45e3d45ea5f7bd01c395ccbde6ae2e750a3ee8ab.zones.catalog2.example. 3600 IN PTR dom14.example.
    update add primaries.45e3d45ea5f7bd01c395ccbde6ae2e750a3ee8ab.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom14.example. is served by secondary and that it's still the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom14.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Deleting domain dom14.example. from catalog2 ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete 45e3d45ea5f7bd01c395ccbde6ae2e750a3ee8ab.zones.catalog2.example. 3600 IN PTR dom14.example.
    update delete primaries.45e3d45ea5f7bd01c395ccbde6ae2e750a3ee8ab.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom14.example. is served by secondary and that it's still the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom14.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing changing label for a member zone"
n=$((n + 1))
echo_i "checking that dom15.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom15.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding a domain dom15.example. to primary ns1 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom15.example.db
echo "@ IN NS invalid." >>ns1/dom15.example.db
rndccmd 10.53.0.1 addzone dom15.example. in default '{type primary; file "dom15.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom15.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom15.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

echo_i "Adding domain dom15.example. to catalog1 zone with 'dom15label1' label ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add dom15label1.zones.catalog1.example. 3600 IN PTR dom15.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

sleep 3

n=$((n + 1))
echo_i "checking that dom15.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom15.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Changing label of domain dom15.example. from 'dom15label1' to 'dom15label2' ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete dom15label1.zones.catalog1.example. 3600 IN PTR dom15.example.
    update add dom15label2.zones.catalog1.example. 3600 IN PTR dom15.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom15.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom15.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "Testing custom properties version '1' and version '2' syntaxes"
n=$((n + 1))
echo_i "checking that dom17.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom17.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom18.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom18.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding domains dom17.example. and dom18.example. to primary ns1 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom17.example.db
echo "@ IN NS invalid." >>ns1/dom17.example.db
echo "@ IN A 192.0.2.1" >>ns1/dom17.example.db
rndccmd 10.53.0.1 addzone dom17.example. in default '{type primary; file "dom17.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom18.example.db
echo "@ IN NS invalid." >>ns1/dom18.example.db
echo "@ IN A 192.0.2.1" >>ns1/dom18.example.db
rndccmd 10.53.0.1 addzone dom18.example. in default '{type primary; file "dom18.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom17.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom17.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom18.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom18.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom17.example. is not served by primary ns3 ($n)"
ret=0
wait_for_no_soa @10.53.0.3 dom17.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom18.example. is not served by primary ns3 ($n)"
ret=0
wait_for_no_soa @10.53.0.3 dom18.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding domains dom17.example. and dom18.example. to primary ns3 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns3/dom17.example.db
echo "@ IN NS invalid." >>ns3/dom17.example.db
echo "@ IN A 192.0.2.2" >>ns3/dom17.example.db
rndccmd 10.53.0.3 addzone dom17.example. '{type primary; file "dom17.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns3/dom18.example.db
echo "@ IN NS invalid." >>ns3/dom18.example.db
echo "@ IN A 192.0.2.2" >>ns3/dom18.example.db
rndccmd 10.53.0.3 addzone dom18.example. '{type primary; file "dom18.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom17.example. is now served by primary ns3 ($n)"
ret=0
wait_for_soa @10.53.0.3 dom17.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom18.example. is now served by primary ns3 ($n)"
ret=0
wait_for_soa @10.53.0.3 dom17.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding domains dom17.example. and dom18.example. to catalog1 zone with ns3 as custom primary using different custom properties syntax ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add dom17.zones.catalog1.example. 3600 IN PTR dom17.example.
    update add dom18.zones.catalog1.example. 3600 IN PTR dom18.example.
    update add primaries.dom17.zones.catalog1.example. 3600 IN A 10.53.0.3
    update add primaries.ext.dom18.zones.catalog1.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: invalid record in catalog zone - primaries.dom17.zones.catalog1.example IN A (failure) - ignoring" \
  && wait_for_message ns2/named.run "catz: adding zone 'dom17.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "catz: adding zone 'dom18.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom17.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" \
  && wait_for_message ns2/named.run "transfer of 'dom18.example/IN/default' from 10.53.0.3#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# The "primaries" custom property for dom17.example. was added using the legacy
# syntax into a version 2 catalog1 zone, so we expect that it was ignored, no
# override of the default setting happened, and dom17.example. was transferred
# from the ns1 primary (the default).
n=$((n + 1))
echo_i "checking that dom17.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom17.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# The "primaries" custom property for dom18.example. was added using a supported
# syntax into a version 2 catalog1 zone, so we expect that it was processed,
# will override the default setting, and dom18.example. was transferred
# from the ns3 primary.
n=$((n + 1))
echo_i "checking that dom18.example. is served by secondary and that it's the one from ns3 ($n)"
ret=0
wait_for_a @10.53.0.2 dom18.example. dig.out.test$n || ret=1
grep "192.0.2.2" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "deleting domain dom17.example. and dom18.example. from catalog1 ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete dom17.zones.catalog1.example. 3600 IN PTR dom17.example.
    update delete dom18.zones.catalog1.example. 3600 IN PTR dom18.example.
    update delete primaries.dom17.zones.catalog1.example. 3600 IN A 10.53.0.3
    update delete primaries.ext.dom18.zones.catalog1.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: catz_delzone_cb: zone 'dom17.example' deleted" \
  && wait_for_message ns2/named.run "catz: catz_delzone_cb: zone 'dom18.example' deleted" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom17.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom17.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom18.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom18.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "adding domains dom17.example. and dom18.example. to catalog2 zone with ns3 as custom primary using different custom properties syntax ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add dom17.zones.catalog2.example. 3600 IN PTR dom17.example.
    update add dom18.zones.catalog2.example. 3600 IN PTR dom18.example.
    update add primaries.dom17.zones.catalog2.example. 3600 IN A 10.53.0.3
    update add primaries.ext.dom18.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: invalid record in catalog zone - primaries.ext.dom18.zones.catalog2.example IN A (failure) - ignoring" \
  && wait_for_message ns2/named.run "catz: adding zone 'dom17.example' from catalog 'catalog2.example'" \
  && wait_for_message ns2/named.run "catz: adding zone 'dom18.example' from catalog 'catalog2.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom17.example/IN/default' from 10.53.0.3#${PORT}: Transfer status: success" \
  && wait_for_message ns2/named.run "transfer of 'dom18.example/IN/default' from 10.53.0.1#${EXTRAPORT1}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# The "primaries" custom property for dom17.example. was added using a supported
# syntax into a version 1 catalog1 zone, so we expect that it was processed,
# will override the default setting, and dom17.example. was transferred
# from the ns3 primary.
n=$((n + 1))
echo_i "checking that dom17.example. is served by secondary and that it's the one from ns3 ($n)"
ret=0
wait_for_a @10.53.0.2 dom17.example. dig.out.test$n || ret=1
grep "192.0.2.2" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# The "primaries" custom property for dom18.example. was added using the new
# syntax into a version 1 catalog1 zone, so we expect that it was ignored, no
# override of the default setting happened, and dom18.example. was transferred
# from the ns1 primary (the default).
n=$((n + 1))
echo_i "checking that dom18.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom18.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n >/dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "deleting domain dom17.example. and dom18.example. from catalog2 ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete dom17.zones.catalog2.example. 3600 IN PTR dom17.example.
    update delete dom18.zones.catalog2.example. 3600 IN PTR dom18.example.
    update delete primaries.dom17.zones.catalog2.example. 3600 IN A 10.53.0.3
    update delete primaries.ext.dom18.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: catz_delzone_cb: zone 'dom17.example' deleted" \
  && wait_for_message ns2/named.run "catz: catz_delzone_cb: zone 'dom18.example' deleted" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom17.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom17.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom18.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom18.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
n=$((n + 1))
echo_i "checking that reconfig can delete and restore catalog zone configuration ($n)"
ret=0
cp ns2/named7.conf ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
cp ns2/named1.conf ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

#########################################################################

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding a dom19.example. to primary via RNDC ($n)"
ret=0
# enough initial content for IXFR response when TXT record is added below
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom19.example.db
echo "@ 3600 IN NS invalid." >>ns1/dom19.example.db
echo "foo 3600 IN TXT some content here" >>ns1/dom19.example.db
echo "bar 3600 IN TXT some content here" >>ns1/dom19.example.db
echo "xxx 3600 IN TXT some content here" >>ns1/dom19.example.db
echo "yyy 3600 IN TXT some content here" >>ns1/dom19.example.db
rndccmd 10.53.0.1 addzone dom19.example. in default '{ type primary; file "dom19.example.db"; allow-transfer { key tsig_key; }; allow-update { any; }; notify explicit; also-notify { 10.53.0.2; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "add an entry to the restored catalog zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 09da0a318e5333a9a7f6c14c385d69f6933e8b72.zones.catalog1.example. 3600 IN PTR dom19.example.
    update add label1.primaries.ext.09da0a318e5333a9a7f6c14c385d69f6933e8b72.zones.catalog1.example. 3600 IN A 10.53.0.1
    update add label1.primaries.ext.09da0a318e5333a9a7f6c14c385d69f6933e8b72.zones.catalog1.example. 3600 IN TXT "tsig_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom19.example' from catalog 'catalog1.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom19.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
n=$((n + 1))
echo_i "Adding a domain tls1.example. to primary via RNDC ($n)"
ret=0
# enough initial content for IXFR response when TXT record is added below
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/tls1.example.db
echo "@ 3600 IN NS invalid." >>ns1/tls1.example.db
echo "foo 3600 IN TXT some content here" >>ns1/tls1.example.db
echo "bar 3600 IN TXT some content here" >>ns1/tls1.example.db
echo "xxx 3600 IN TXT some content here" >>ns1/tls1.example.db
echo "yyy 3600 IN TXT some content here" >>ns1/tls1.example.db
rndccmd 10.53.0.1 addzone tls1.example. in default '{ type primary; file "tls1.example.db"; allow-transfer transport tls { key tsig_key; }; allow-update { any; }; notify explicit; also-notify { 10.53.0.4; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that tls1.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 tls1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns4/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain tls1.example. to catalog-tls zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 1ba056ba375209a66a2c9a0617b1df714b998112.zones.catalog-tls.example. 3600 IN PTR tls1.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns4/named.run "catz: adding zone 'tls1.example' from catalog 'catalog-tls.example'" \
  && wait_for_message ns4/named.run "transfer of 'tls1.example/IN' from 10.53.0.1#${TLSPORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that tls1.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.4 tls1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
# GL #5658

n=$((n + 1))
echo_i "Adding a domain longlong.longlong.long.long.name.example. to primary via RNDC ($n)"
ret=0
# enough initial content for IXFR response when TXT record is added below
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/longlong.longlong.long.long.name.example.db
echo "@ 3600 IN NS invalid." >>ns1/longlong.longlong.long.long.name.example.db
echo "foo 3600 IN TXT some content here" >>ns1/longlong.longlong.long.long.name.example.db
echo "bar 3600 IN TXT some content here" >>ns1/longlong.longlong.long.long.name.example.db
echo "xxx 3600 IN TXT some content here" >>ns1/longlong.longlong.long.long.name.example.db
echo "yyy 3600 IN TXT some content here" >>ns1/longlong.longlong.long.long.name.example.db
rndccmd 10.53.0.1 addzone longlong.longlong.long.long.name.example. in default '{ type primary; file "longlong.longlong.long.long.name.example.db"; allow-transfer { key longlonglongname0123456789abcdef; }; allow-update { any; }; notify explicit; also-notify { 10.53.0.4; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that longlong.longlong.long.long.name.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 longlong.longlong.long.long.name.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns4/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain longlong.longlong.long.long.name.example. to catalog-misc zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add longlong.zones.catalog-misc.example. 3600 IN PTR longlong.longlong.long.long.name.example.
    update add label1.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN A 10.53.0.1
    update add label1.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN TXT "longlonglongname0123456789abcdef"
    update add label2.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN A 10.53.0.1
    update add label2.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN TXT "longlonglongname0123456789abcdef"
    update add label3.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN A 10.53.0.1
    update add label3.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN TXT "longlonglongname0123456789abcdef"
    update add label4.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN A 10.53.0.1
    update add label4.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN TXT "longlonglongname0123456789abcdef"
    update add label5.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN A 10.53.0.1
    update add label5.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN TXT "longlonglongname0123456789abcdef"
    update add label6.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN A 10.53.0.1
    update add label6.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN TXT "longlonglongname0123456789abcdef"
    update add label7.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN A 10.53.0.1
    update add label7.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN TXT "longlonglongname0123456789abcdef"
    update add label8.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN A 10.53.0.1
    update add label8.primaries.ext.longlong.zones.catalog-misc.example. 3600 IN TXT "longlonglongname0123456789abcdef"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns4/named.run "catz: adding zone 'longlong.longlong.long.long.name.example' from catalog 'catalog-misc.example'" \
  && wait_for_message ns4/named.run "transfer of 'longlong.longlong.long.long.name.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that longlong.longlong.long.long.name.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.4 longlong.longlong.long.long.name.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
# GL #3777
nextpart ns4/named.run >/dev/null

n=$((n + 1))
echo_i "Adding domain self.example. to catalog-self zone without updating the serial ($n)"
ret=0
echo "self.zones.catalog-self.example. 3600 IN PTR self.example." >>ns4/catalog-self.example.db
rndccmd 10.53.0.4 reload || ret=1

n=$((n + 1))
echo_i "Issuing another rndc reload command after 1 second ($n)"
sleep 1
rndccmd 10.53.0.4 reload || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding a domain dom20.example. to primary via RNDC ($n)"
ret=0
# enough initial content for IXFR response when TXT record is added below
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom20.example.db
echo "@ 3600 IN NS invalid." >>ns1/dom20.example.db
echo "foo 3600 IN TXT some content here" >>ns1/dom20.example.db
echo "bar 3600 IN TXT some content here" >>ns1/dom20.example.db
echo "xxx 3600 IN TXT some content here" >>ns1/dom20.example.db
echo "yyy 3600 IN TXT some content here" >>ns1/dom20.example.db
rndccmd 10.53.0.1 addzone dom20.example. in default '{ type primary; file "dom20.example.db"; allow-update { any; }; notify explicit; also-notify { 10.53.0.2; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom20.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom20.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding domain dom20.example. to catalog6 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add dom20.zones.catalog6.example. 3600 IN PTR dom20.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
start=$(date +%s)
wait_for_message ns2/named.run "catz: adding zone 'dom20.example' from catalog 'catalog6.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom20.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
end=$(date +%s)
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that 'notify-defer 5;' worked ($n)"
ret=0
elapsed=$(($end - $start))
[ $elapsed -ge 5 ] || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom20.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom20.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns2/named.run >/dev/null

n=$((n + 1))
echo_i "Adding a domain dom21.example. to primary via RNDC ($n)"
ret=0
# enough initial content for IXFR response when TXT record is added below
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" >ns1/dom21.example.db
echo "@ 3600 IN NS invalid." >>ns1/dom21.example.db
echo "foo 3600 IN TXT some content here" >>ns1/dom21.example.db
echo "bar 3600 IN TXT some content here" >>ns1/dom21.example.db
echo "xxx 3600 IN TXT some content here" >>ns1/dom21.example.db
echo "yyy 3600 IN TXT some content here" >>ns1/dom21.example.db
rndccmd 10.53.0.1 addzone dom21.example. in default '{ type primary; file "dom21.example.db"; allow-update { any; }; notify explicit; also-notify { 10.53.0.2; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom21.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom21.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Adding domain dom21.example. to catalog6 zone ($n)"
ret=0
$NSUPDATE -d <<END >>nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add dom21.zones.catalog6.example. 3600 IN PTR dom21.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Sending 'rndc notify catalog6.example.' to primary via RNDC ($n)"
ret=0
rndccmd 10.53.0.1 notify catalog6.example. || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
start=$(date +%s)
wait_for_message ns2/named.run "catz: adding zone 'dom21.example' from catalog 'catalog6.example'" \
  && wait_for_message ns2/named.run "transfer of 'dom21.example/IN/default' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
end=$(date +%s)
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that 'notify-defer 5;' was ignored because of implicit notify command ($n)"
ret=0
elapsed=$(($end - $start))
[ $elapsed -lt 5 ] || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that dom21.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom21.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status + ret))

##########################################################################
echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
