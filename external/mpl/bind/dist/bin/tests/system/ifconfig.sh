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

#
# Set up interface aliases for bind9 system tests.
#
# IPv4: 10.53.0.{1..11}				RFC 1918
#       10.53.1.{1..2}
#       10.53.2.{1..2}
# IPv6: fd92:7065:b8e:ffff::{1..11}		ULA
#       fd92:7065:b8e:99ff::{1..2}
#       fd92:7065:b8e:ff::{1..2}
#
# We also set the MTU on the 1500 bytes to match the default MTU on physical
# interfaces, so we can properly test the cases with packets bigger than
# interface MTU.

SYSTEMTESTTOP="$(cd -P -- "$(dirname -- "$0")" && pwd -P)"
. "$SYSTEMTESTTOP/conf.sh"

export SYSTEMTESTTOP

sys=$($SHELL "$TOP/config.guess")

use_ip=
case "$sys" in
        *-*-linux*)
                if type ip > /dev/null; then
                        use_ip=yes
                elif type ifconfig > /dev/null; then
                        :
                else
                        echo "$0: can't find ip or ifconfig" >&2
                        exit 1
                fi
                ;;
esac

up() {
	case "$sys" in
	    *-pc-solaris2.5.1)
		[ "$a" ] && ifconfig lo0:$int $a netmask 0xffffffff up
		;;
	    *-sun-solaris2.[6-7])
		[ "$a" ] && ifconfig lo0:$int $a netmask 0xffffffff up
		;;
	    *-*-solaris2.[8-9]|*-*-solaris2.10)
		[ "$a" ] && {
			/sbin/ifconfig lo0:$int plumb
			/sbin/ifconfig lo0:$int $a up
			/sbin/ifconfig lo0:$int mtu 1500
		}
		[ "$aaaa" ] && {
			/sbin/ifconfig lo0:$int inet6 plumb
			/sbin/ifconfig lo0:$int inet6 $aaaa up
		}
		;;
	    *-*-solaris2.1[1-9])
		[ "$a" ] && {
			/sbin/ipadm create-addr -t -T static \
				    -a $a lo0/bind9v4$int ||
			echo failed lo0/bind9v4$int
		}
		[ "$aaaa" ] && {
			/sbin/ipadm create-addr -t -T static \
				    -a $aaaa lo0/bind9v6$int ||
			echo failed lo0/bind9v6$int
		}
	       ;;
	    *-*-linux*)
		if [ "$use_ip" ]; then
			ip address add $a/24 dev lo:$int
			ip link set dev lo:$int mtu 1500
			[ "$aaaa" ] && ip address add $aaaa/64 dev lo
		else
			ifconfig lo:$int $a up netmask 255.255.255.0 mtu 1500
			[ "$aaaa" ] && ifconfig lo inet6 add $aaaa/64
		fi
		;;
	    *-unknown-freebsd*)
		[ "$a" ] && ifconfig lo0 $a alias netmask 0xffffffff mtu 1500
		[ "$aaaa" ] && ifconfig lo0 inet6 $aaaa alias
		;;
	    *-unknown-dragonfly*|*-unknown-netbsd*|*-unknown-openbsd*)
		[ "$a" ] && ifconfig lo0 $a alias netmask 255.255.255.0 mtu 1500
		[ "$aaaa" ] && ifconfig lo0 inet6 $aaaa alias
		;;
	    *-*-bsdi[3-5].*)
		[ "$a" ] && ifconfig lo0 add $a netmask 255.255.255.0
		;;
	    *-dec-osf[4-5].*)
		[ "$a" ] && ifconfig lo0 alias $a
		;;
	    *-sgi-irix6.*)
		[ "$a" ] && ifconfig lo0 alias $a
		;;
	    *-*-sysv5uw7*|*-*-sysv*UnixWare*|*-*-sysv*OpenUNIX*)
		[ "$a" ] && ifconfig lo0 $a alias netmask 0xffffffff
		;;
	    *-ibm-aix4.*|*-ibm-aix5.*)
		[ "$a" ] && ifconfig lo0 alias $a
		[ "$aaaa" ] && ifconfig lo0 inet6 alias -dad $aaaa/64
		;;
	    hpux)
		[ "$a" ] && ifconfig lo0:$int $a netmask 255.255.255.0 up
		[ "$aaaa" ] && ifconfig lo0:$int inet6 $aaaa up
		;;
	    *-sco3.2v*)
		[ "$a" ] && ifconfig lo0 alias $a
		;;
	    *-darwin*)
		[ "$a" ] && ifconfig lo0 alias $a
		[ "$aaaa" ] && ifconfig lo0 inet6 $aaaa alias
		;;
	    *-cygwin*)
		echo "Please run ifconfig.bat as Administrator."
		exit 1
		;;
	    *)
		echo "Don't know how to set up interface.  Giving up."
		exit 1
		;;
	esac
}

down() {
	case "$sys" in
	    *-pc-solaris2.5.1)
		[ "$a" ] && ifconfig lo0:$int 0.0.0.0 down
		;;
	    *-sun-solaris2.[6-7])
		[ "$a" ] && ifconfig lo0:$int $a down
		;;
	    *-*-solaris2.[8-9]|*-*-solaris2.10)
		[ "$a" ] && {
			ifconfig lo0:$int $a down
			ifconfig lo0:$int $a unplumb
		}
		[ "$aaaa" ] && {
			ifconfig lo0:$int inet6 down
			ifconfig lo0:$int inet6 unplumb
		}
		;;
	    *-*-solaris2.1[1-9])
		[ "$a" ] && {
			ipadm delete-addr lo0/bind9v4$int ||
			echo failed lo0/bind9v4$int
		}
		[ "$aaaa" ] && {
			ipadm delete-addr lo0/bind9v6$int ||
			echo failed lo0/bind9v6$int
		}
		;;

	    *-*-linux*)
		if [ "$use_ip" ]; then
			[ "$a" ] && ip address del $a/24 dev lo:$int
			[ "$aaaa" ] && ip address del $aaaa/64 dev lo
		else
			[ "$a" ] && ifconfig lo:$int $a down
			[ "$aaaa" ] && ifconfig lo inet6 del $aaaa/64
		fi
		;;
	    *-unknown-freebsd*)
		[ "$a" ] && ifconfig lo0 $a delete
		[ "$aaaa" ] && ifconfig lo0 inet6 $aaaa delete
		;;
	    *-unknown-netbsd*)
		[ "$a" ] && ifconfig lo0 $a delete
		[ "$aaaa" ] && ifconfig lo0 inet6 $aaaa delete
		;;
	    *-unknown-openbsd*)
		[ "$a" ] && ifconfig lo0 $a delete
		[ "$aaaa" ] && ifconfig lo0 inet6 $aaaa delete
		;;
	    *-*-bsdi[3-5].*)
		[ "$a" ] && ifconfig lo0 remove $a
		;;
	    *-dec-osf[4-5].*)
		[ "$a" ] && ifconfig lo0 -alias $a
		;;
	    *-sgi-irix6.*)
		[ "$a" ] && ifconfig lo0 -alias $a
		;;
	    *-*-sysv5uw7*|*-*-sysv*UnixWare*|*-*-sysv*OpenUNIX*)
		[ "$a" ] && ifconfig lo0 -alias $a
		;;
	    *-ibm-aix4.*|*-ibm-aix5.*)
		[ "$a" ] && ifconfig lo0 delete $a
		[ "$aaaa" ] && ifconfig lo0 delete inet6 $aaaa/64
		;;
	    hpux)
		[ "$a" ] && ifconfig lo0:$int 0.0.0.0
		[ "$aaaa" ] && ifconfig lo0:$int inet6 ::
		;;
	    *-sco3.2v*)
		[ "$a" ] && ifconfig lo0 -alias $a
		;;
	    *darwin*)
		[ "$a" ] && ifconfig lo0 -alias $a
		[ "$aaaa" ] && ifconfig lo0 inet6 $aaaa delete
		;;
	    *-cygwin*)
		echo "Please run ifconfig.bat as Administrator."
		exit 1
		;;
	    *)
		echo "Don't know how to destroy interface.  Giving up."
		exit 1
		;;
	esac
}

sequence() (
	awk -v s=$1 -v e=$2 '
		BEGIN {
			for (i = s ; i <= e; i++) { print i; }
			exit;
		}'
)

#
# 'max', 'i' and 'ns' are used to compute the interface identifier for
# systems that need it and must be unique for each interface (e.g. lo:$int).
#
#	int=$((i * max + ns))
#
# 'max' is the number of nameservers configured in the inner loop.
# 'i' is the outer loop counter.
# 'ns' in the namserver being configured.
# 'int' interface identifier.
#
max=11
case $1 in
    start|up|stop|down)
	for i in $(sequence 0 2)
	do
		case $i in
		  0) ipv6="ff" ;;
		  1) ipv6="99" ;;
		  2) ipv6="00" ;;
		  *) ipv6="" ;;
		esac
		for ns in $(sequence 1 $max)
		do
			[ $i -gt 0 -a $ns -gt 2 ] && break
			int=$((i * max + ns))
			a=10.53.$i.$ns
			aaaa=fd92:7065:b8e:${ipv6}ff::$ns
			case "$1" in
			    start|up) up;;
			    stop|down) down;;
			esac
		done
	done
	;;
    *)
	echo "Usage: $0 { up | down }"
	exit 1
	;;
esac
