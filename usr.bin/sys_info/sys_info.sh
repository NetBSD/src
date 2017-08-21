#! /bin/sh

# $NetBSD: sys_info.sh,v 1.6 2017/08/21 19:36:57 agc Exp $

# Copyright (c) 2016 Alistair Crooks <agc@NetBSD.org>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

SYS_INFO_VERSION=20170821
LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-/usr/lib:/usr/X11R7/lib}

# short script to look for an executable $2, and if found, to place
# path in $1
# taken from pkgsrc bootstrap
which_prog()
{
	_var="$1"; _name="$2"

	eval _tmp=\"\$$_var\"
	if [ "x$_tmp" != "x" ]; then
		# Variable is already set (by the user, for example)
		return 0
	fi

	for _d in `echo $PATH | tr ':' ' '`; do
		if [ -f "$_d/$_name" ] && [ -x "$_d/$_name" ]; then
			# Program found
			eval $_var=\""$_d/$_name"\"
			return 0
		fi
	done

	return 1
}

# print out the version for the given argument
getversion() {
	case "$1" in
	awk)
		awk --version | awk '{ print $1 "-" $3 }'
		;;
	bind|named)
		named -v | awk '{ gsub("-", "", $2); gsub("P", "pl", $2); print tolower($1) "-" $2 }'
		;;
	bzip2)
		bzip2 --help 2>&1 | awk '{ sub(",", "", $7); print "bzip2-" $7; exit }'
		;;
	calendar)
		v=$(calendar -v 2>/dev/null || true)
		case "${v}" in
		"")	echo "calendar-20150701" ;;
		*)	echo ${v} ;;
		esac
		;;
	ftpd)
		strings -a /usr/libexec/ftpd | awk '$1 == "NetBSD-ftpd" { print "ftpd-" $2 }'
		;;
	g++|c++)
		g++ --version | awk '{ print $1 "-" $4; exit }'
		;;
	gcc|cc)
		gcc --version | awk '{ print $1 "-" $4; exit }'
		;;
	grep)
		grep --version | awk '{ print $1 "-" $4 $5; exit }'
		;;
	gzip)
		gzip --version 2>&1 | awk '{ print $2 "-" $3 }'
		;;
	httpd|bozohttpd)
		v=$(/usr/libexec/httpd -G 2>/dev/null || true)
		case "${v}" in
		"")
			strings -a /usr/libexec/httpd | awk -F/ '$1 == "bozohttpd" && NF == 2 { print $1 "-" $2; exit }'
			;;
		*)
			echo bozohttpd-${v##*/}
			;;
		esac
		;;
	lib*)
		dlist=$(echo ${LD_LIBRARY_PATH} | awk '{ gsub(":", " "); print }')
		for d in ${dlist}; do
			if [ -e ${d}/$1.so ]; then
				ls -al ${d}/$1.so | awk '{ sub(".*/", "", $11); sub("\\.so\\.", "-", $11); print $11 }'
				break
			fi
		done
		;;
	netbsd)
		uname -sr | awk '{ print $1 "-" $2 }'
		;;
	netpgp)
		netpgp -V | awk '{ sub("/.*", "", $3); print "netpgp-" $3; exit }'
		;;
	netpgpverify)
		netpgpverify -v | awk '{ print $1 "-" $3 }'
		;;
	ntp)
		ntpq --version | awk '{ sub("-.", ""); sub("p", "pl", $2); print "ntp-" $2 }'
		;;
	openssl)
		openssl version 2>/dev/null | awk '{ print tolower($1) "-" $2 }'
		;;
	sqlite|sqlite3)
		sqlite3 --version | awk '{ print "sqlite3-" $1 }'
		;;
	ssh|openssh)
		ssh -V 2>&1 | awk '{ sub("_", "-", $1); print tolower($1) }'
		;;
	sshd)
		sshd -V 2>&1 | awk '/OpenSSH/ { sub("_", "D-", $1); print tolower($1) }'
		;;
	sys_info)
		echo sys_info-${SYS_INFO_VERSION}
		;;
	tcsh)
		if which_prog tcshpath tcsh; then
			${tcshpath} --version | awk '{ print $1 "-" $2 }'
		else
			echo "tcsh: not found"
		fi
		;;
	unbound)
		if which_prog unboundpath unbound-control; then
			${unboundpath} -h | awk '/^Version/ { print "unbound-" $2 }'
		else
			echo "unbound: not found"
		fi
		;;
	xz)
		xz --version | awk '{ print $1 "-" $4; exit }'
		;;
	esac
}

# check if we have our only option
while [ $# -gt 0 ]; do
	case "$1" in
	-v)	set -x ;;
	*)	break ;;
	esac
	shift
done

all=false
# if no arg specified, we want them all
if [ $# -eq 0 ]; then
	all=true
fi

# if we want to do every one, then let's get the arguments
# not really scalable
if ${all}; then
	args='awk bind bzip2 calendar ftpd g++ gcc grep gzip httpd netbsd netpgp'
	args="${args} netpgpverify ntp openssl sqlite ssh sshd sys_info tcsh"
	args="${args} unbound xz"
else
	args=$(echo $@ | tr ' ' '\n' | sort | uniq)
fi
set -- ${args}

while [ $# -gt 0 ]; do
	getversion $1
	shift
done
