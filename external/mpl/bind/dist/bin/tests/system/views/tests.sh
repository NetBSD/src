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

# shellcheck source=conf.sh
. ../conf.sh

dig_with_opts() {
  "$DIG" +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd +noauth -p "${PORT}" "$@"
}

dig_with_shortopts() {
  "$DIG" +tcp +short -p "${PORT}" "$@"
}

RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

status=0

echo_i "fetching a.example from ns2's initial configuration"
dig_with_opts a.example. @10.53.0.2 any >dig.out.ns2.1 || status=1

echo_i "fetching a.example from ns3's initial configuration"
dig_with_opts a.example. @10.53.0.3 any >dig.out.ns3.1 || status=1

echo_i "copying in new configurations for ns2 and ns3"
rm -f ns2/named.conf ns3/named.conf ns2/example.db
cp -f ns2/example2.db ns2/example.db
cp ns2/named2.conf ns2/named.conf
cp ns3/named2.conf ns3/named.conf

echo_i "reloading ns2 and ns3 with rndc"
nextpart ns2/named.run >/dev/null
nextpart ns3/named.run >/dev/null
rndc_reload ns2 10.53.0.2
rndc_reload ns3 10.53.0.3

echo_i "wait for reload to complete"
ret=0
_check_reload() (
  nextpartpeek ns2/named.run | grep "all zones loaded" >/dev/null \
    && nextpartpeek ns3/named.run | grep "all zones loaded" >/dev/null \
    && nextpartpeek ns3/named.run | grep "zone_dump: zone example/IN: enter" >/dev/null
)
retry_quiet 10 _check_reload || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "fetching a.example from ns2's 10.53.0.4, source address 10.53.0.4"
dig_with_opts -b 10.53.0.4 a.example. @10.53.0.4 any >dig.out.ns4.2 || status=1

echo_i "fetching a.example from ns2's 10.53.0.2, source address 10.53.0.2"
dig_with_opts -b 10.53.0.2 a.example. @10.53.0.2 any >dig.out.ns2.2 || status=1

echo_i "fetching a.example from ns3's 10.53.0.3, source address defaulted"
dig_with_opts @10.53.0.3 a.example. any >dig.out.ns3.2 || status=1

echo_i "comparing ns3's initial a.example to one from reconfigured 10.53.0.2"
digcomp dig.out.ns3.1 dig.out.ns2.2 || status=1

echo_i "comparing ns3's initial a.example to one from reconfigured 10.53.0.3"
digcomp dig.out.ns3.1 dig.out.ns3.2 || status=1

echo_i "comparing ns2's initial a.example to one from reconfigured 10.53.0.4"
digcomp dig.out.ns2.1 dig.out.ns4.2 || status=1

echo_i "comparing ns2's initial a.example to one from reconfigured 10.53.0.3"
echo_i "(should be different)"
if $PERL ../digcomp.pl dig.out.ns2.1 dig.out.ns3.2 >/dev/null; then
  echo_i "no differences found.  something's wrong."
  status=1
fi

echo_i "updating cloned zone in internal view"
$NSUPDATE <<EOF
server 10.53.0.2 ${PORT}
zone clone
update add b.clone. 300 in a 10.1.0.3
send
EOF
echo_i "sleeping to allow update to take effect"
sleep 5

echo_i "verifying update affected both views"
ret=0
one=$(dig_with_shortopts -b 10.53.0.2 @10.53.0.2 b.clone a)
two=$(dig_with_shortopts -b 10.53.0.4 @10.53.0.2 b.clone a)
if [ "$one" != "$two" ]; then
  echo_i "'$one' does not match '$two'"
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "verifying forwarder in cloned zone works"
ret=0
one=$(dig_with_shortopts -b 10.53.0.2 @10.53.0.2 child.clone txt)
two=$(dig_with_shortopts -b 10.53.0.4 @10.53.0.2 child.clone txt)
three=$(dig_with_shortopts @10.53.0.3 child.clone txt)
four=$(dig_with_shortopts @10.53.0.5 child.clone txt)
echo "$three" | grep NS3 >/dev/null || {
  ret=1
  echo_i "expected response from NS3 got '$three'"
}
echo "$four" | grep NS5 >/dev/null || {
  ret=1
  echo_i "expected response from NS5 got '$four'"
}
if [ "$one" = "$two" ]; then
  echo_i "'$one' matches '$two'"
  ret=1
fi
if [ "$one" != "$three" ]; then
  echo_i "'$one' does not match '$three'"
  ret=1
fi
if [ "$two" != "$four" ]; then
  echo_i "'$two' does not match '$four'"
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "verifying inline zones work with views"
ret=0
wait_for_signed() {
  "$DIG" -p "${PORT}" @10.53.0.2 -b 10.53.0.2 +dnssec DNSKEY inline >dig.out.internal
  "$DIG" -p "${PORT}" @10.53.0.2 -b 10.53.0.5 +dnssec DNSKEY inline >dig.out.external
  grep "ANSWER: 3," dig.out.internal >/dev/null || return 1
  grep "ANSWER: 3," dig.out.external >/dev/null || return 1
  return 0
}
retry_quiet 10 wait_for_signed || ret=1
int=$(awk '$4 == "DNSKEY" { print $8 }' dig.out.internal | sort)
ext=$(awk '$4 == "DNSKEY" { print $8 }' dig.out.external | sort)
test "$int" != "$ext" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "verifying adding of multiple inline zones followed by reconfiguration works"

[ ! -f ns2/zones.conf ] && touch ns2/zones.conf
cp ns2/named3.conf ns2/named.conf

i=1
while [ $i -lt 50 ]; do
  ret=0
  zone_name=$(printf "example%03d.com" $i)

  # Add a new zone to the configuration.
  cat >>ns2/zones.conf <<-EOF
	zone "${zone_name}" {
	    type primary;
	    file "db.${zone_name}";
	    dnssec-policy default;
	    inline-signing yes;
	};
	EOF

  # Create a master file for the zone.
  cat >"ns2/db.${zone_name}" <<-EOF
	\$TTL 86400
	@		IN	SOA	localhost. hostmaster.localhost (
						1612542642 ; serial
						12H ; refresh
						1H  ; retry
						2w  ; expiry
						1h  ; minimum
					)
	@		IN      NS      localhost
	localhost       IN      A       127.0.0.1
	EOF

  $RNDCCMD 10.53.0.2 reconfig || ret=1
  if [ $ret != 0 ]; then
    echo_i "failed"
    break
  fi
  i=$((i + 1))
done
status=$((status + ret))

echo_i "exit status: $status"
[ "$status" -eq 0 ] || exit 1
