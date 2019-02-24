#!/bin/sh

. $SYSTEMTESTTOP/conf.sh

$SHELL clean.sh
copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf
