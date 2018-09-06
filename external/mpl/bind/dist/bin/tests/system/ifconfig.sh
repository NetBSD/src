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

#
# Set up interface aliases for bind9 system tests.
#
# IPv4: 10.53.0.{1..8}				RFC 1918
#       10.53.1.{0..2}
#       10.53.2.{0..2}
# IPv6: fd92:7065:b8e:ffff::{1..8}		ULA
#       fd92:7065:b8e:99ff::{1..2}
#       fd92:7065:b8e:ff::{1..2}
#

config_guess=""
for f in ./config.guess ../../../config.guess
do
	if test -f $f
	then
		config_guess=$f
	fi
done

if test "X$config_guess" = "X"
then
	cat <<EOF >&2
$0: must be run from the top level source directory or the
bin/tests/system directory
EOF
	exit 1
fi

# If running on hp-ux, don't even try to run config.guess.
# It will try to create a temporary file in the current directory,
# which fails when running as root with the current directory
# on a NFS mounted disk.

case `uname -a` in
	*HP-UX*) sys=hpux ;;
	*) sys=`sh $config_guess` ;;
esac

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

case "$1" in

    start|up)
	for i in 0 1 2
	do
		case $i in
		  0) ipv6="ff" ;;
		  1) ipv6="99" ;;
		  2) ipv6="00" ;;
		  *) ipv6="" ;;
		esac
		for ns in 1 2 3 4 5 6 7 8
		do
			[ $i -gt 0 -a $ns -gt 2 ] && break
			int=`expr $i \* 10 + $ns - 1`
			case "$sys" in
			    *-pc-solaris2.5.1)
				ifconfig lo0:$int 10.53.$i.$ns \
					netmask 0xffffffff up
				;;
			    *-sun-solaris2.[6-7])
				ifconfig lo0:$int 10.53.$i.$ns \
					netmask 0xffffffff up
				;;
			    *-*-solaris2.[8-9]|*-*-solaris2.1[0-9])
				/sbin/ifconfig lo0:$int plumb
				/sbin/ifconfig lo0:$int 10.53.$i.$ns up
				/sbin/ifconfig lo0:$int inet6 plumb
				[ "$ipv6" ] && /sbin/ifconfig lo0:$int \
					inet6 fd92:7065:b8e:${ipv6}ff::$ns up
				;;
			    *-*-linux*)
                                if [ $use_ip ]; then
                                        ip address add 10.53.$i.$ns/24 \
                                            dev lo:$int
                                        [ "$ipv6" ] && ip address add \
                                            fd92:7065:b8e:${ipv6}ff::$ns/64 \
                                            dev lo
                                else
                                        ifconfig lo:$int 10.53.$i.$ns up \
                                                netmask 255.255.255.0
                                        [ "$ipv6" ] && ifconfig lo inet6 add \
                                                fd92:7065:b8e:${ipv6}ff::$ns/64
                                fi
				;;
			    *-unknown-freebsd*)
				ifconfig lo0 10.53.$i.$ns alias \
					netmask 0xffffffff
				[ "$ipv6" ] && ifconfig lo0 inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns alias
				;;
			    *-unknown-netbsd*)
				ifconfig lo0 10.53.$i.$ns alias \
					netmask 255.255.255.0
				[ "$ipv6" ] && ifconfig lo0 inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns alias
				;;
			    *-unknown-openbsd*)
				ifconfig lo0 10.53.$i.$ns alias \
					netmask 255.255.255.0
				[ "$ipv6" ] && ifconfig lo0 inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns alias
				;;
			    *-*-bsdi[3-5].*)
				ifconfig lo0 add 10.53.$i.$ns \
					netmask 255.255.255.0
				;;
			    *-dec-osf[4-5].*)
				ifconfig lo0 alias 10.53.$i.$ns
				;;
			    *-sgi-irix6.*)
				ifconfig lo0 alias 10.53.$i.$ns
				;;
			    *-*-sysv5uw7*|*-*-sysv*UnixWare*|*-*-sysv*OpenUNIX*)
				ifconfig lo0 10.53.$i.$ns alias \
					netmask 0xffffffff
				;;
			    *-ibm-aix4.*|*-ibm-aix5.*)
				ifconfig lo0 alias 10.53.$i.$ns
				[ "$ipv6" ] && ifconfig lo0 inet6 alias -dad \
					fd92:7065:b8e:${ipv6}ff::$ns/64
				;;
			    hpux)
				ifconfig lo0:$int 10.53.$i.$ns \
					netmask 255.255.255.0 up
				[ "$ipv6" ] && ifconfig lo0:$int inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns up
				;;
			    *-sco3.2v*)
				ifconfig lo0 alias 10.53.$i.$ns
				;;
			    *-darwin*)
				ifconfig lo0 alias 10.53.$i.$ns
				[ "$ipv6" ] && ifconfig lo0 inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns alias
				;;
			    *-cygwin*)
			        echo "Please run ifconfig.bat as Administrator."
			        exit 1
			        ;;
			    *)
				echo "Don't know how to set up interface.  Giving up."
				exit 1
			esac
		done
	done
	;;

    stop|down)
	for i in 0 1 2
	do
		case $i in
		  0) ipv6="ff" ;;
		  1) ipv6="99" ;;
		  2) ipv6="00" ;;
		  *) ipv6="" ;;
		esac
		for ns in 8 7 6 5 4 3 2 1
		do
			[ $i -gt 0 -a $ns -gt 2 ] && continue
			int=`expr $i \* 10 + $ns - 1`
			case "$sys" in
			    *-pc-solaris2.5.1)
				ifconfig lo0:$int 0.0.0.0 down
				;;
			    *-sun-solaris2.[6-7])
				ifconfig lo0:$int 10.53.$i.$ns down
				;;
			    *-*-solaris2.[8-9]|*-*-solaris2.1[0-9])
				ifconfig lo0:$int 10.53.$i.$ns down
				ifconfig lo0:$int 10.53.$i.$ns unplumb
				ifconfig lo0:$int inet6 down
				ifconfig lo0:$int inet6 unplumb
				;;
			    *-*-linux*)
                                if [ $use_ip ]; then
                                        ip address del 10.53.$i.$ns/24 \
                                            dev lo:$int
                                        [ "$ipv6" ] && ip address del \
                                            fd92:7065:b8e:${ipv6}ff::$ns/64 \
                                            dev lo
                                else
                                        ifconfig lo:$int 10.53.$i.$ns down
                                        [ "$ipv6" ] && ifconfig lo inet6 \
                                            del fd92:7065:b8e:${ipv6}ff::$ns/64
                                fi
				;;
			    *-unknown-freebsd*)
				ifconfig lo0 10.53.$i.$ns delete
				[ "$ipv6" ] && ifconfig lo0 inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns delete
				;;
			    *-unknown-netbsd*)
				ifconfig lo0 10.53.$i.$ns delete
				[ "$ipv6" ] && ifconfig lo0 inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns delete
				;;
			    *-unknown-openbsd*)
				ifconfig lo0 10.53.$i.$ns delete
				[ "$ipv6" ] && ifconfig lo0 inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns delete
				;;
			    *-*-bsdi[3-5].*)
				ifconfig lo0 remove 10.53.$i.$ns
				;;
			    *-dec-osf[4-5].*)
				ifconfig lo0 -alias 10.53.$i.$ns
				;;
			    *-sgi-irix6.*)
				ifconfig lo0 -alias 10.53.$i.$ns
				;;
			    *-*-sysv5uw7*|*-*-sysv*UnixWare*|*-*-sysv*OpenUNIX*)
				ifconfig lo0 -alias 10.53.$i.$ns
				;;
			    *-ibm-aix4.*|*-ibm-aix5.*)
				ifconfig lo0 delete 10.53.$i.$ns
				[ "$ipv6" ] && ifconfig lo0 delete inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns/64
				;;
			    hpux)
				ifconfig lo0:$int 0.0.0.0
				ifconfig lo0:$int inet6 ::
				;;
			    *-sco3.2v*)
				ifconfig lo0 -alias 10.53.$i.$ns
				;;
			    *darwin*)
				ifconfig lo0 -alias 10.53.$i.$ns
				[ "$ipv6" ] && ifconfig lo0 inet6 \
					fd92:7065:b8e:${ipv6}ff::$ns delete
				;;
			    *-cygwin*)
			        echo "Please run ifconfig.bat as Administrator."
			        exit 1
			        ;;
			    *)
				echo "Don't know how to destroy interface.  Giving up."
				exit 1
			esac
		done
	done
	;;

	*)
		echo "Usage: $0 { up | down }"
		exit 1
esac
