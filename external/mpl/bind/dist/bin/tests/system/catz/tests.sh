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
SYSTEMTESTTOP=..
. "$SYSTEMTESTTOP/conf.sh"

dig_with_opts() {
	"$DIG" -p "${PORT}" "$@"
}

rndccmd() (
	"$RNDC" -c "$SYSTEMTESTTOP/common/rndc.conf" -p "${CONTROLPORT}" -s "$@"
)

_wait_for_message() (
	nextpartpeek "$1" > wait_for_message.$n
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
echo_i "Testing adding/removing of domain in catalog zone"
n=$((n+1))
echo_i "checking that dom1.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain dom1.example. to primary via RNDC ($n)"
ret=0
# enough initial content for IXFR response when TXT record is added below
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom1.example.db
echo "@ 3600 IN NS invalid." >> ns1/dom1.example.db
echo "foo 3600 IN TXT some content here" >> ns1/dom1.example.db
echo "bar 3600 IN TXT some content here" >> ns1/dom1.example.db
echo "xxx 3600 IN TXT some content here" >> ns1/dom1.example.db
echo "yyy 3600 IN TXT some content here" >> ns1/dom1.example.db
rndccmd 10.53.0.1 addzone dom1.example. '{ type primary; file "dom1.example.db"; allow-update { any; }; notify explicit; also-notify { 10.53.0.2; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom1.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Adding domain dom1.example. to catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add e721433b6160b450260d4f54b3ec8bab30cb3b83.zones.catalog1.example. 3600 IN PTR dom1.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom1.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run "transfer of 'dom1.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom1.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that zone-directory is populated ($n)"
ret=0
wait_for_zonefile "ns2/zonedir/__catz___default_catalog1.example_dom1.example.db" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "update dom1.example. ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update add dom1.example 0 IN TXT added record
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "wait for secondary to be updated ($n)"
ret=0
wait_for_txt() {
	dig_with_opts @10.53.0.2 TXT dom1.example. > dig.out.test$n || return 1
	grep "ANSWER: 1," dig.out.test$n > /dev/null || return 1
	grep "status: NOERROR" dig.out.test$n > /dev/null || return 1
	grep "IN.TXT." dig.out.test$n > /dev/null || return 1
}
retry_quiet 10 wait_for_txt || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check that journal was created for cleanup test ($n)"
ret=0
test -f ns2/zonedir/__catz___default_catalog1.example_dom1.example.db.jnl || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "update catalog zone serial ($n)"
ret=0
# default minimum update rate is once / 5 seconds
sleep 5
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add catalog1.example 3600 SOA . . 20 86400 3600 86400 3600
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "wait for catalog zone to transfer ($n)"
ret=0
wait_for_soa_equal_20() {
	dig_with_opts @10.53.0.2 SOA catalog1.example. > dig.out.test$n || return 1
	grep "ANSWER: 1," dig.out.test$n > /dev/null || return 1
	grep "status: NOERROR" dig.out.test$n > /dev/null || return 1
	grep 'IN.SOA.\. \. 20 ' dig.out.test$n > /dev/null || return 1
}
retry_quiet 10 wait_for_soa_equal_20 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "update dom1.example. again ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update add foo.dom1.example 0 IN TXT added record
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "wait for secondary to be updated again ($n)"
ret=0
wait_for_txt() {
	dig_with_opts @10.53.0.2 TXT foo.dom1.example. > dig.out.test$n || return 1
	grep "ANSWER: 2," dig.out.test$n > /dev/null || return 1
	grep "status: NOERROR" dig.out.test$n > /dev/null || return 1
	grep "IN.TXT." dig.out.test$n > /dev/null || return 1
}
retry_quiet 10 wait_for_txt || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "removing domain dom1.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update delete e721433b6160b450260d4f54b3ec8bab30cb3b83.zones.catalog1.example
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "zone_shutdown: zone dom1.example/IN: shutting down" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom1.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom1.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that zone-directory is emptied ($n)"
ret=0
wait_for_no_zonefile "ns2/zonedir/__catz___default_catalog1.example_dom1.example.db" || ret=1
wait_for_no_zonefile "ns2/zonedir/__catz___default_catalog1.example_dom1.example.db.jnl" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
echo_i "Testing various simple operations on domains, including using multiple catalog zones and garbage in zone"
n=$((n+1))
echo_i "adding domain dom2.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom2.example.db
echo "@ IN NS invalid." >> ns1/dom2.example.db
rndccmd 10.53.0.1 addzone dom2.example. '{type primary; file "dom2.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "adding domain dom4.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom4.example.db
echo "@ IN NS invalid." >> ns1/dom4.example.db
rndccmd 10.53.0.1 addzone dom4.example. '{type primary; file "dom4.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "adding domains dom2.example, dom3.example. and some garbage to catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR dom2.example.
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
status=$((status+ret))

n=$((n+1))
echo_i "adding domain dom4.example. to catalog2 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add de26b88d855397a03f77ff1162fd055d8b419584.zones.catalog2.example. 3600 IN PTR dom4.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))


n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: updating catalog zone 'catalog2.example' with serial 2670950425" &&
wait_for_message ns2/named.run "catz: adding zone 'dom2.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run "catz: adding zone 'dom3.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run "catz: adding zone 'dom4.example' from catalog 'catalog2.example'" &&
wait_for_message ns2/named.run "transfer of 'dom4.example/IN' from 10.53.0.1#${EXTRAPORT1}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom4.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom4.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))


n=$((n+1))
echo_i "checking that dom3.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "adding a domain dom3.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom3.example.db
echo "@ IN NS invalid." >> ns1/dom3.example.db
rndccmd 10.53.0.1 addzone dom3.example. '{type primary; file "dom3.example.db"; also-notify { 10.53.0.2; }; notify explicit; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom3.example. is served by primary ($n)"
ret=0
wait_for_soa  @10.53.0.1 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom2.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run "catz: adding zone 'dom3.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "transfer of 'dom2.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" &&
wait_for_message ns2/named.run  "transfer of 'dom3.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom3.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "reconfiguring secondary - checking if catz survives a certain class of failed reconfiguration attempts ($n)"
ret=0
sed -e "s/^#T3//" < ns2/named1.conf.in > ns2/named.conf.tmp
copy_setports ns2/named.conf.tmp ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p "${CONTROLPORT}" reconfig > /dev/null 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking again that dom3.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom3.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "reconfiguring secondary - reverting the bad configuration ($n)"
ret=0
copy_setports ns2/named1.conf.in ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "removing all records from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete 636722929740e507aaf27c502812fc395d30fb17.zones.catalog1.example. 3600 IN PTR dom2.example.
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
    send

END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "removing all records from catalog2 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete de26b88d855397a03f77ff1162fd055d8b419584.zones.catalog2.example. 3600 IN PTR dom4.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
echo_i "Testing masters suboption and random labels"
n=$((n+1))
echo_i "adding dom5.example. with a valid masters suboption (IP without TSIG) and a random label ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add somerandomlabel.zones.catalog1.example. 3600 IN PTR dom5.example.
    update add masters.somerandomlabel.zones.catalog1.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom5.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "transfer of 'dom5.example/IN' from 10.53.0.3#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom5.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom5.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "removing dom5.example. ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete somerandomlabel.zones.catalog1.example. 3600 IN PTR dom5.example.
    update delete masters.somerandomlabel.zones.catalog1.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "zone_shutdown: zone dom5.example/IN: shutting down" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom5.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom5.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))


##########################################################################
echo_i "Testing masters global option"
n=$((n+1))
echo_i "adding dom6.example. and a valid global masters option (IP without TSIG) ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add masters.catalog1.example. 3600 IN A 10.53.0.3
    update add masters.catalog1.example. 3600 IN AAAA fd92:7065:b8e:ffff::3
    update add 4346f565b4d63ddb99e5d2497ff22d04e878e8f8.zones.catalog1.example. 3600 IN PTR dom6.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom6.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "transfer of 'dom6.example/IN' from " > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom6.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom6.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "removing dom6.example. ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete masters.catalog1.example. 3600 IN A 10.53.0.3
    update delete masters.catalog1.example. 3600 IN AAAA  fd92:7065:b8e:ffff::3
    update delete 4346f565b4d63ddb99e5d2497ff22d04e878e8f8.zones.catalog1.example. 3600 IN PTR dom6.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "zone_shutdown: zone dom6.example/IN: shutting down" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom6.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom6.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "adding dom6.example. and an invalid global masters option (TSIG without IP) ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add label1.masters.catalog1.example. 3600 IN TXT "tsig_key"
    update add 4346f565b4d63ddb99e5d2497ff22d04e878e8f8.zones.catalog1.example. 3600 IN PTR dom6.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom6.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "error \"failure\" while trying to generate config for zone \"dom6.example\"" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "removing dom6.example. ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete label1.masters.catalog1.example. 3600 IN TXT "tsig_key"
    update delete 4346f565b4d63ddb99e5d2497ff22d04e878e8f8.zones.catalog1.example. 3600 IN PTR dom6.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: deleting zone 'dom6.example' from catalog 'catalog1.example' - success" > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
n=$((n+1))
echo_i "Checking that a missing zone directory forces in-memory ($n)"
ret=0
grep "'nonexistent' not found; zone files will not be saved" ns2/named.run > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
echo_i "Testing allow-query and allow-transfer ACLs"
n=$((n+1))
echo_i "adding domains dom7.example. and dom8.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom7.example.db
echo "@ IN NS invalid." >> ns1/dom7.example.db
rndccmd 10.53.0.1 addzone dom7.example. '{type primary; file "dom7.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom8.example.db
echo "@ IN NS invalid." >> ns1/dom8.example.db
rndccmd 10.53.0.1 addzone dom8.example. '{type primary; file "dom8.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom7.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom7.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "adding domain dom7.example. to catalog1 zone with an allow-query statement ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 78833ec3c0059fd4540fee81c7eaddce088e7cd7.zones.catalog1.example. 3600 IN PTR dom7.example.
    update add allow-query.78833ec3c0059fd4540fee81c7eaddce088e7cd7.zones.catalog1.example. 3600 IN APL 1:10.53.0.1/32 !1:10.53.0.0/30 1:0.0.0.0/0
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom7.example' from catalog 'catalog1.example'" > /dev/null &&
wait_for_message ns2/named.run  "transfer of 'dom7.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom7.example. is accessible from 10.53.0.1 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom7.example. dig.out.test$n -b 10.53.0.1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom7.example. is not accessible from 10.53.0.2 ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom7.example. dig.out.test$n -b 10.53.0.2 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom7.example. is accessible from 10.53.0.5 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom7.example. dig.out.test$n -b 10.53.0.5 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null
n=$((n+1))
echo_i "adding dom8.example. domain and global allow-query and allow-transfer ACLs ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add cba95222e308baba42417be6021026fdf20827b6.zones.catalog1.example. 3600 IN PTR dom8.example
    update add allow-query.catalog1.example. 3600 IN APL 1:10.53.0.1/32
    update add allow-transfer.catalog1.example. 3600 IN APL 1:10.53.0.2/32
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" &&
wait_for_message ns2/named.run  "transfer of 'dom8.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom8.example. is accessible from 10.53.0.1 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom8.example. dig.out.test$n -b 10.53.0.1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom8.example. is not accessible from 10.53.0.2 ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom8.example. dig.out.test$n -b 10.53.0.2 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom8.example. is not AXFR accessible from 10.53.0.1 ($n)"
ret=0
dig_with_opts @10.53.0.2 axfr dom8.example. -b 10.53.0.1 > dig.out.test$n
grep "Transfer failed." dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom8.example. is AXFR accessible from 10.53.0.2 ($n)"
ret=0
dig_with_opts @10.53.0.2 axfr dom8.example. -b 10.53.0.2 > dig.out.test$n
grep -v "Transfer failed." dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null
n=$((n+1))
echo_i "deleting global allow-query and allow-domain ACLs ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete allow-query.catalog1.example. 3600 IN APL 1:10.53.0.1/32
    update delete allow-transfer.catalog1.example. 3600 IN APL 1:10.53.0.2/32
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom8.example. is accessible from 10.53.0.1 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom8.example. dig.out.test$n -b 10.53.0.1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom8.example. is accessible from 10.53.0.2 ($n)"
ret=0
wait_for_soa @10.53.0.2 dom8.example. dig.out.test$n -b 10.53.0.2 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom8.example. is AXFR accessible from 10.53.0.1 ($n)"
ret=0
dig_with_opts @10.53.0.2 axfr dom8.example. -b 10.53.0.1 > dig.out.test$n
grep -v "Transfer failed." dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom8.example. is AXFR accessible from 10.53.0.2 ($n)"
ret=0
dig_with_opts @10.53.0.2 axfr dom8.example. -b 10.53.0.2 > dig.out.test$n
grep -v "Transfer failed." dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))


##########################################################################
echo_i "Testing TSIG keys for masters set per-domain"
n=$((n+1))
echo_i "adding a domain dom9.example. to primary via RNDC, with transfers allowed only with TSIG key ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom9.example.db
echo "@ IN NS invalid." >> ns1/dom9.example.db
rndccmd 10.53.0.1 addzone dom9.example. '{type primary; file "dom9.example.db"; allow-transfer { key tsig_key; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom9.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "adding domain dom9.example. to catalog1 zone with a valid masters suboption (IP with TSIG) ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN PTR dom9.example.
    update add label1.masters.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN A 10.53.0.1
    update add label1.masters.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "tsig_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom9.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "transfer of 'dom9.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom9.example. is accessible on secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "change TSIG key name on primary ($n)"
ret=0
rndccmd 10.53.0.1 modzone dom9.example. '{type primary; notify yes; file "dom9.example.db"; allow-transfer { key next_key; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "update TSIG key name in catalog zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update del label1.masters.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "tsig_key"
    update add label1.masters.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "next_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: modifying zone 'dom9.example' from catalog 'catalog1.example'" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "update zone contents and reload ($n)"
ret=0
echo "@ 3600 IN SOA . . 2 3600 3600 3600 3600" > ns1/dom9.example.db
echo "@ IN NS ns2" >> ns1/dom9.example.db
echo "ns2 IN A 10.53.0.2" >> ns1/dom9.example.db
rndccmd 10.53.0.1 reload dom9.example. || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "wait for primary to update zone ($n)"
ret=0
wait_for_a @10.53.0.1 ns2.dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "wait for secondary to update zone ($n)"
ret=0
wait_for_a @10.53.0.2 ns2.dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "deleting domain dom9.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN PTR dom9.example.
    update delete label1.masters.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN A 10.53.0.1
    update delete label1.masters.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "next_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: deleting zone 'dom9.example' from catalog 'catalog1.example' - success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom9.example. is no longer accessible on secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom9.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "adding domain dom9.example. to catalog1 zone with an invalid masters suboption (TSIG without IP) ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN PTR dom9.example.
    update add label1.masters.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "tsig_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom9.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "error \"failure\" while trying to generate config for zone \"dom9.example\"" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "deleting domain dom9.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN PTR dom9.example.
    update delete label1.masters.f0f989bc71c5c8ca3a1eb9c9ab5246521907e3af.zones.catalog1.example. 3600 IN TXT "tsig_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: deleting zone 'dom9.example' from catalog 'catalog1.example'" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
echo_i "Testing catalog entries that can't be represented as filenames"
# note: we need 4 backslashes in the shell to get 2 backslashes in DNS
# presentation format, which is 1 backslash on the wire.
for special in \
       this.is.a.very.very.long.long.long.domain.that.will.cause.catalog.zones.to.generate.hash.instead.of.using.regular.filename.dom10.example \
       this.zone/domain.has.a.slash.dom10.example \
       this.zone\\\\domain.has.backslash.dom10.example \
       this.zone:domain.has.a.colon.dom.10.example
do
    # hashes below are generated by:
    # python ${TOP}/contrib/scripts/catzhash.py "${special}"

    case "$special" in
    this.is.a.very.very.long.long.long.domain.that.will.cause.catalog.zones.to.generate.hash.instead.of.using.regular.filename.dom10.example)
        hash=825f48b1ce1b4cf5a041d20255a0c8e98d114858
        db=__catz__4d70696f2335687069467f11f5d5378c480383f97782e553fb2d04a7bb2a23ed.db
        ;;
    this.zone/domain.has.a.slash.dom10.example)
        hash=e64cc64c99bf52d0a77fb16dd7ed57cf925a36aa
        db=__catz__46ba3e1b28d5955e5313d5fee61bedc78c71d08035aa7ea2f7bf0b8228ab3acc.db
        ;;
    this.zone\\\\domain.has.backslash.dom10.example)
        hash=91e27e02153d38cf656a9b376d7747fbcd19f985
        db=__catz__b667f7ff802c0895e0506699951cff9a1cab68c5ef8546aa0d07425f244ed870.db
        ;;
    this.zone:domain.has.a.colon.dom.10.example)
        hash=8b7238bf4c34045834c573ba4116557ebb24d33c
        db=__catz__5c721f7872913a4e7fa8ad42589cce5dd6e551a4c9e6ab3f86e77c0bbc7c2ca6.db
        ;;
    esac

    n=$((n+1))
    echo_i "checking that ${special}. is not served by primary ($n)"
    ret=0
    wait_for_no_soa @10.53.0.1 "${special}" dig.out.test$n || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "Adding a domain ${special}. to primary via RNDC ($n)"
    ret=0
    echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom10.example.db
    echo "@ IN NS invalid." >> ns1/dom10.example.db
    rndccmd 10.53.0.1 addzone '"'"${special}"'"' '{type primary; file "dom10.example.db";};' || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "checking that ${special}. is now served by primary ($n)"
    ret=0
    wait_for_soa @10.53.0.1 "${special}." dig.out.test$n || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    nextpart ns2/named.run >/dev/null

    n=$((n+1))
    echo_i "Adding domain ${special}. to catalog1 zone ($n)"
    ret=0
    $NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
      server 10.53.0.1 ${PORT}
      update add ${hash}.zones.catalog1.example 3600 IN PTR ${special}.
      send
END
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "waiting for secondary to sync up ($n)"
    ret=0
    wait_for_message ns2/named.run  "catz: adding zone '$special' from catalog 'catalog1.example'" &&
    wait_for_message ns2/named.run  "transfer of '$special/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "checking that ${special}. is served by secondary ($n)"
    ret=0
    wait_for_soa @10.53.0.2 "${special}." dig.out.test$n || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "checking that zone-directory is populated with a hashed filename ($n)"
    ret=0
    wait_for_zonefile "ns2/zonedir/$db" || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "removing domain ${special}. from catalog1 zone ($n)"
    ret=0
    $NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
      server 10.53.0.1 ${PORT}
      update delete ${hash}.zones.catalog1.example
      send
END
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "waiting for secondary to sync up ($n)"
    ret=0
    wait_for_message ns2/named.run  "zone_shutdown: zone ${special}/IN: shutting down" || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "checking that ${special}. is not served by secondary ($n)"
    ret=0
    wait_for_no_soa @10.53.0.2 "${special}." dig.out.test$n || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "checking that zone-directory is emptied ($n)"
    ret=0
    wait_for_no_zonefile "ns2/zonedir/$db" || ret=1
    wait_for_no_zonefile "ns2/zonedir/$db.jnl" || ret=1
    if [ $ret -ne 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
done

##########################################################################
echo_i "Testing adding a domain and a subdomain of it"
n=$((n+1))
echo_i "checking that dom11.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain dom11.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom11.example.db
echo "@ IN NS invalid." >> ns1/dom11.example.db
rndccmd 10.53.0.1 addzone dom11.example. '{type primary; file "dom11.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom11.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Adding domain dom11.example. to catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 0580d70e769c86c8b951a488d8b776627f427d7a.zones.catalog1.example. 3600 IN PTR dom11.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom11.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "transfer of 'dom11.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom11.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that subdomain.of.dom11.example. is not served by primary ($n)"
ret=0
wait_for_rcode NXDOMAIN SOA @10.53.0.1 subdomain.of.dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain subdomain.of.dom11.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/subdomain.of.dom11.example.db
echo "@ IN NS invalid." >> ns1/subdomain.of.dom11.example.db
rndccmd 10.53.0.1 addzone subdomain.of.dom11.example. '{type primary; file "subdomain.of.dom11.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that subdomain.of.dom11.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 subdomain.of.dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Adding domain subdomain.of.dom11.example. to catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 25557e0bdd10cb3710199bb421b776df160f241e.zones.catalog1.example. 3600 IN PTR subdomain.of.dom11.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'subdomain.of.dom11.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "transfer of 'subdomain.of.dom11.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that subdomain.of.dom11.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 subdomain.of.dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "removing domain dom11.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update delete 0580d70e769c86c8b951a488d8b776627f427d7a.zones.catalog1.example
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "zone_shutdown: zone dom11.example/IN: shutting down" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom11.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that subdomain.of.dom11.example. is still served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 subdomain.of.dom11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "removing domain subdomain.of.dom11.example. from catalog1 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
   server 10.53.0.1 ${PORT}
   update delete 25557e0bdd10cb3710199bb421b776df160f241e.zones.catalog1.example
   send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "zone_shutdown: zone subdomain.of.dom11.example/IN: shutting down" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that subdomain.of.dom11.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 subdomain.of.d11.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
echo_i "Testing adding a catalog zone at runtime with rndc reconfig"
n=$((n+1))
echo_i "checking that dom12.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain dom12.example. to primary via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom12.example.db
echo "@ IN NS invalid." >> ns1/dom12.example.db
rndccmd 10.53.0.1 addzone dom12.example. '{type primary; file "dom12.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom12.example. is now served by primary ($n)"
ret=0
wait_for_soa @10.53.0.1 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Adding domain dom12.example. to catalog4 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 871d51e5433543c0f6fb263c40f359fbc152c8ae.zones.catalog4.example. 3600 IN PTR dom12.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom12.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))


n=$((n+1))
echo_i "reconfiguring secondary - adding catalog4 catalog zone ($n)"
ret=0
sed -e "s/^#T1//g" <  ns2/named1.conf.in > ns2/named.conf.tmp
copy_setports ns2/named.conf.tmp ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom12.example' from catalog 'catalog4.example'" &&
wait_for_message ns2/named.run  "transfer of 'dom12.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom7.example. is still served by secondary after reconfiguration ($n)"
ret=0
wait_for_soa @10.53.0.2 dom7.example. dig.out.test$n -b 10.53.0.1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))
n=$((n+1))

echo_i "checking that dom12.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "reconfiguring secondary - removing catalog4 catalog zone, adding non-existent catalog5 catalog zone ($n)"
ret=0
sed -e "s/^#T2//" < ns2/named1.conf.in > ns2/named.conf.tmp
copy_setports ns2/named.conf.tmp ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p "${CONTROLPORT}" reconfig > /dev/null 2>&1 && ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "reconfiguring secondary - removing non-existent catalog5 catalog zone ($n)"
ret=0
copy_setports ns2/named1.conf.in ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom12.example. is not served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom12.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "removing domain dom12.example. from catalog4 zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete 871d51e5433543c0f6fb263c40f359fbc152c8ae.zones.catalog4.example. 3600 IN PTR dom12.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
echo_i "Testing having a zone in two different catalogs"
n=$((n+1))
echo_i "checking that dom13.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom13.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain dom13.example. to primary ns1 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom13.example.db
echo "@ IN NS invalid." >> ns1/dom13.example.db
echo "@ IN A 192.0.2.1" >> ns1/dom13.example.db
rndccmd 10.53.0.1 addzone dom13.example. '{type primary; file "dom13.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom13.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom13.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain dom13.example. to primary ns3 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns3/dom13.example.db
echo "@ IN NS invalid." >> ns3/dom13.example.db
echo "@ IN A 192.0.2.2" >> ns3/dom13.example.db
rndccmd 10.53.0.3 addzone dom13.example. '{type primary; file "dom13.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom13.example. is now served by primary ns3 ($n)"
ret=0
wait_for_soa @10.53.0.3 dom13.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))


nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Adding domain dom13.example. to catalog1 zone with ns1 as primary ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog1.example. 3600 IN PTR dom13.example.
    update add masters.8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog1.example. 3600 IN A 10.53.0.1
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom13.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "transfer of 'dom13.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "checking that dom13.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom13.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding domain dom13.example. to catalog2 zone with ns3 as primary ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add 8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog2.example. 3600 IN PTR dom13.example.
    update add masters.8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom13.example. is served by secondary and that it's still the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom13.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Deleting domain dom13.example. from catalog2 ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete 8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog2.example. 3600 IN PTR dom13.example.
    update delete masters.8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom13.example. is served by secondary and that it's still the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom13.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Deleting domain dom13.example. from catalog1 ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete 8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog1.example. 3600 IN PTR dom13.example.
    update delete masters.8d7989c746b3f92b3bba2479e72afd977198363f.zones.catalog1.example. 3600 IN A 10.53.0.2
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom13.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom13.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
echo_i "Testing having a regular zone and a zone in catalog zone of the same name"
n=$((n+1))
echo_i "checking that dom14.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom14.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain dom14.example. to primary ns1 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom14.example.db
echo "@ IN NS invalid." >> ns1/dom14.example.db
echo "@ IN A 192.0.2.1" >> ns1/dom14.example.db
rndccmd 10.53.0.1 addzone dom14.example. '{type primary; file "dom14.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom14.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom14.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain dom14.example. to primary ns3 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns3/dom14.example.db
echo "@ IN NS invalid." >> ns3/dom14.example.db
echo "@ IN A 192.0.2.2" >> ns3/dom14.example.db
rndccmd 10.53.0.3 addzone dom14.example. '{type primary; file "dom14.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom14.example. is now served by primary ns3 ($n)"
ret=0
wait_for_soa @10.53.0.3 dom14.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Adding domain dom14.example. with rndc with ns1 as primary ($n)"
ret=0
rndccmd 10.53.0.2 addzone dom14.example. '{type secondary; primaries {10.53.0.1;};};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "transfer of 'dom14.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "checking that dom14.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom14.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding domain dom14.example. to catalog2 zone with ns3 as primary ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update add 45e3d45ea5f7bd01c395ccbde6ae2e750a3ee8ab.zones.catalog2.example. 3600 IN PTR dom14.example.
    update add masters.45e3d45ea5f7bd01c395ccbde6ae2e750a3ee8ab.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom14.example. is served by secondary and that it's still the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom14.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Deleting domain dom14.example. from catalog2 ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.3 ${PORT}
    update delete 45e3d45ea5f7bd01c395ccbde6ae2e750a3ee8ab.zones.catalog2.example. 3600 IN PTR dom14.example.
    update delete masters.45e3d45ea5f7bd01c395ccbde6ae2e750a3ee8ab.zones.catalog2.example. 3600 IN A 10.53.0.3
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom14.example. is served by secondary and that it's still the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom14.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
echo_i "Testing changing label for a member zone"
n=$((n+1))
echo_i "checking that dom15.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom15.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain dom15.example. to primary ns1 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom15.example.db
echo "@ IN NS invalid." >> ns1/dom15.example.db
rndccmd 10.53.0.1 addzone dom15.example. '{type primary; file "dom15.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom15.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom15.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

echo_i "Adding domain dom15.example. to catalog1 zone with 'dom15label1' label ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add dom15label1.zones.catalog1.example. 3600 IN PTR dom15.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

sleep 3

n=$((n+1))
echo_i "checking that dom15.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom15.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Changing label of domain dom15.example. from 'dom15label1' to 'dom15label2' ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete dom15label1.zones.catalog1.example. 3600 IN PTR dom15.example.
    update add dom15label2.zones.catalog1.example. 3600 IN PTR dom15.example.
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom15.example. is served by secondary ($n)"
ret=0
wait_for_soa @10.53.0.2 dom15.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##########################################################################
echo_i "Testing recreation of a manually deleted zone after a reload"
n=$((n+1))
echo_i "checking that dom16.example. is not served by primary ($n)"
ret=0
wait_for_no_soa @10.53.0.1 dom16.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "Adding a domain dom16.example. to primary ns1 via RNDC ($n)"
ret=0
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom16.example.db
echo "@ IN NS invalid." >> ns1/dom16.example.db
echo "@ IN A 192.0.2.1" >> ns1/dom16.example.db
rndccmd 10.53.0.1 addzone dom16.example. '{type primary; file "dom16.example.db";};' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom16.example. is now served by primary ns1 ($n)"
ret=0
wait_for_soa @10.53.0.1 dom16.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Adding domain dom16.example. to catalog1 zone with ns1 as primary ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add efe725d0cf430ffb113b9bcf59266f066a21216b.zones.catalog1.example. 3600 IN PTR dom16.example.
    update add masters.efe725d0cf430ffb113b9bcf59266f066a21216b.zones.catalog1.example. 3600 IN A 10.53.0.1
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: adding zone 'dom16.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run  "transfer of 'dom16.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "checking that dom16.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom16.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

echo_i "Deleting dom16.example. from secondary ns2 via RNDC ($n)"
ret=0
rndccmd 10.53.0.2 delzone dom16.example. >/dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom16.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom16.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

echo_i "Reloading secondary ns2 via RNDC ($n)"
ret=0
rndccmd 10.53.0.2 reload >/dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom16.example. is served by secondary and that it's the one from ns1 ($n)"
ret=0
wait_for_a @10.53.0.2 dom16.example. dig.out.test$n || ret=1
grep "192.0.2.1" dig.out.test$n > /dev/null || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Deleting domain dom16.example. from catalog1 ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update delete efe725d0cf430ffb113b9bcf59266f066a21216b.zones.catalog1.example. 3600 IN PTR dom16.example.
    update delete masters.efe725d0cf430ffb113b9bcf59266f066a21216b.zones.catalog1.example. 3600 IN A 10.53.0.1
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run  "catz: update_from_db: new zone merged" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that dom16.example. is no longer served by secondary ($n)"
ret=0
wait_for_no_soa @10.53.0.2 dom16.example. dig.out.test$n || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that reconfig can delete and restore catalog zone configuration ($n)"
ret=0
copy_setports ns2/named2.conf.in ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
copy_setports ns2/named1.conf.in ns2/named.conf
rndccmd 10.53.0.2 reconfig || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#########################################################################

nextpart ns2/named.run >/dev/null

n=$((n+1))
echo_i "Adding a dom19.example. to primary via RNDC ($n)"
ret=0
# enough initial content for IXFR response when TXT record is added below
echo "@ 3600 IN SOA . . 1 3600 3600 3600 3600" > ns1/dom19.example.db
echo "@ 3600 IN NS invalid." >> ns1/dom19.example.db
echo "foo 3600 IN TXT some content here" >> ns1/dom19.example.db
echo "bar 3600 IN TXT some content here" >> ns1/dom19.example.db
echo "xxx 3600 IN TXT some content here" >> ns1/dom19.example.db
echo "yyy 3600 IN TXT some content here" >> ns1/dom19.example.db
rndccmd 10.53.0.1 addzone dom19.example. '{ type primary; file "dom19.example.db"; allow-transfer { key tsig_key; }; allow-update { any; }; notify explicit; also-notify { 10.53.0.2; }; };' || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "add an entry to the restored catalog zone ($n)"
ret=0
$NSUPDATE -d <<END >> nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    update add 09da0a318e5333a9a7f6c14c385d69f6933e8b72.zones.catalog1.example. 3600 IN PTR dom19.example.
    update add label1.masters.09da0a318e5333a9a7f6c14c385d69f6933e8b72.zones.catalog1.example. 3600 IN A 10.53.0.1
    update add label1.masters.09da0a318e5333a9a7f6c14c385d69f6933e8b72.zones.catalog1.example. 3600 IN TXT "tsig_key"
    send
END
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "waiting for secondary to sync up ($n)"
ret=0
wait_for_message ns2/named.run "catz: adding zone 'dom19.example' from catalog 'catalog1.example'" &&
wait_for_message ns2/named.run "transfer of 'dom19.example/IN' from 10.53.0.1#${PORT}: Transfer status: success" || ret=1
if [ $ret -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
