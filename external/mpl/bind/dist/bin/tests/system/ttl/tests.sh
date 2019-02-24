#!/bin/sh

. $SYSTEMTESTTOP/conf.sh

dig_with_options() { "$DIG" +noadd +nosea +nostat +noquest +nocomm +nocmd -p "${PORT}" "$@"; }

status=0
t=0

echo_i "testing min-cache-ttl"
t=$((t+1))
dig_with_options IN SOA min-example. @10.53.0.2 > dig.out.${t}
TTL=$(< dig.out.${t} awk '{ print $2; }')
[ "$TTL" -eq 60 ] || status=$((status+1))

echo_i "testing min-ncache-ttl"
t=$((t+1))
dig_with_options IN MX min-example. @10.53.0.2 > dig.out.${t}
TTL=$(< dig.out.${t} awk '{ print $2; }')
[ "$TTL" -eq 30 ] || status=$((status+1))

echo_i "testing max-cache-ttl"
t=$((t+1))
dig_with_options IN SOA max-example. @10.53.0.2 > dig.out.${t}
TTL=$(< dig.out.${t} awk '{ print $2; }')
[ "$TTL" -eq 120 ] || status=$((status+1))

echo_i "testing max-ncache-ttl"
t=$((t+1))
dig_with_options IN MX max-example. @10.53.0.2 > dig.out.${t}
TTL=$(< dig.out.${t} awk '{ print $2; }')
[ "$TTL" -eq 60 ] || status=$((status+1))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
