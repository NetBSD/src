#! /bin/sh

# $NetBSD: sys_info.sh,v 1.17 2017/09/28 18:08:04 agc Exp $

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

SYS_INFO_VERSION=20170928

PATH=$(sysctl -n user.cs_path)
export PATH

LIBRARY_PATH=${LD_LIBRARY_PATH:-/usr/lib:/usr/X11R7/lib}

# default libraries when no args are given (sorted...)
LIBS=
LIBS="${LIBS} libc"
LIBS="${LIBS} libcurses"
LIBS="${LIBS} libdrm"
LIBS="${LIBS} libm"
LIBS="${LIBS} libresolv"
LIBS="${LIBS} libsqlite"
LIBS="${LIBS} libssh"
LIBS="${LIBS} libstdc++"
LIBS="${LIBS} libterminfo"
LIBS="${LIBS} libutil"
LIBS="${LIBS} libX11"
LIBS="${LIBS} libXaw7"
LIBS="${LIBS} libXcb"
LIBS="${LIBS} libXfont"
LIBS="${LIBS} libXft"
LIBS="${LIBS} libXrandr"
LIBS="${LIBS} libXt"

# short script to look for an executable $2, and if found, to place
# path in $1
# taken from pkgsrc bootstrap
which_prog()
{
	local IFS _var _name _d -
	set -f

	_var="$1"; _name="$2"

	eval _d=\"\$$_var\"
	if [ -n "$_d" ]; then
		# Variable is already set (by the user, for example)
		return 0
	fi

	IFS=:
	for _d in $PATH ; do
		if [ -f "$_d/$_name" ] && [ -x "$_d/$_name" ]; then
			# Program found
			eval $_var=\""$_d/$_name"\"
			return 0
		fi
	done

	return 1
}

savedIFS=unset
saveIFS() { savedIFS="${IFS-unset}"; IFS="$1"; }
restIFS() { test "${savedIFS}" = unset && unset IFS || IFS="${savedIFS}"; }

run() {
	 # must send to stderr, as run is used in $() sometimes.
	 $verbose && printf >&2 '%s\n' "${PS4:-...: }${1}"
	 eval "$1"
}

# print out the version for the given argument (or everything)

# case patterns are sorted by output order so
#	sys_info
# and
#	sys_info | sort -f
# generate identical output

getversion() {
	case "$1" in
	'')
		$all || return 0 ;&
	awk)
		run "awk --version | awk '{ print \$1 \"-\" \$3 }'"
		$all || return 0 ;&
	[Bb][Ii][Nn][Dd]|named)
		run "named -v | awk '{ gsub(\"-\", \"\", \$2); gsub(\"P\", \"pl\", \$2); print tolower(\$1) \"-\" \$2 }'"
		$all || return 0 ;&
	bozohttpd|httpd)
		v=$(run "${destdir}/usr/libexec/httpd -G" 2>/dev/null)
		case "${v}" in
		"")
			run  "strings -a ${destdir}/usr/libexec/httpd | awk -F/ '\$1 == \"bozohttpd\" && NF == 2 { print \$1 \"-\" \$2; exit }'"
			;;
		*)
			printf '%s\n' "bozohttpd-${v##*/}"
			;;
		esac
		$all || return 0 ;&
	bzip2)
		run  "bzip2 --help 2>&1 | awk '{ sub(\",\", \"\", \$7); print \"bzip2-\" \$7; exit }'"
		$all || return 0 ;&
	calendar)
		v=$(run "calendar -v" 2>/dev/null || true)
		case "${v}" in
		"")	printf '%s\n' "calendar-20150701" ;;
		*)	printf '%s\n' "${v}" ;;
		esac
		$all || return 0 ;&
	dhcpcd)
		run  "dhcpcd --version | sed -e 's/ /-/g' -e 1q"
		$all || return 0 ;&
	dtc)
		run "dtc --version | sed 's/Version: DTC /dtc-/'"
		$all || return 0 ;&
	ftpd)
		run "strings -a /usr/libexec/ftpd | awk '\$1 == \"NetBSD-ftpd\" { print \"ftpd-\" \$2 }'"
		$all || return 0 ;&
	g++|c++)
		run "g++ --version | awk '{ print \$1 \"-\" \$4; exit }'"
		$all || return 0 ;&
	gcc|cc)
		run "gcc --version | awk '{ print \$1 \"-\" \$4; exit }'"
		$all || return 0 ;&
	grep)
		run "grep --version | awk '{ print \$1 \"-\" \$4 \$5; exit }'"
		$all || return 0 ;&
	gzip)
		run "gzip --version 2>&1 | awk '{ print \$2 \"-\" \$3 }'"
		$all || return 0 ;&
	lib*)
		for L in ${1:-$LIBS}; do
			saveIFS :
			for d in ${LIBRARY_PATH} nowhere; do
				restIFS
				if [ -e ${d}/$L.so ]; then
					run "ls -al \"${d}/$L.so\" | sed -e 's/^.*-> //' -e 's;^.*/;;' -e 's/\\.so\\./-/'"
					break
				fi
			done
			restIFS
			test "$d" = nowhere && test -n "$1" &&
				printf 2>&1 '%s\n' "$0: library $1 not found"
		done
		$all || return 0 ;&
	[Nn]et[Bb][Ss][Dd]|kernel)
		run "uname -sr | awk '{ print \$1 \"-\" \$2 }'"
		$all || return 0 ;&
	netpgp)
		run "netpgp -V | awk '{ sub(\"/.*\", \"\", \$3); print \"netpgp-\" \$3; exit }'"
		$all || return 0 ;&
	netpgpverify)
		run "netpgpverify -v | awk '{ print \$1 \"-\" \$3 }'"
		$all || return 0 ;&
	ntp)
		run "ntpq --version | awk '{ sub(\"-.\", \"\"); sub(\"p\", \"pl\", \$2); print \"ntp-\" \$2 }'"
		$all || return 0 ;&
	openssh|ssh)
		run "ssh -V 2>&1 | awk '{ sub(\"_\", \"-\", \$1); print tolower(\$1) }'"
		$all || return 0 ;&
	opensshd|sshd)
		run "sshd -V 2>&1 | awk '/OpenSSH/ { sub(\"_\", \"D-\", \$1); print tolower(\$1) }'"
		$all || return 0 ;&
	openssl)
		run "openssl version 2>/dev/null | awk '{ print tolower(\$1) \"-\" \$2 }'"
		$all || return 0 ;&
	pcap)
		if which_prog tcpdumppath tcpdump; then
			run "${tcpdumppath} -h 2>&1 | awk '\$1 == \"libpcap\" { sub(\" version \", \"-\"); print }'"
		fi
		$all || return 0 ;&
	pkg_info|pkg_install)
		if which_prog infopath pkg_info; then
			run "printf 'pkg_install-%s\n' \$(${infopath} -V)"
		fi
		$all || return 0 ;&
	sh)
		run "set -- \$NETBSD_SHELL; case \"\$1+\$2\" in *+BUILD*) ;; +) set -- ancient;; *) set -- \"\$1\";;esac; printf 'sh-%s\\n' \$1\${2:+-\${2#BUILD:}}"
		$all || return 0 ;&
	sqlite|sqlite3)
		run "sqlite3 --version | awk '{ print \"sqlite3-\" \$1 }'"
		$all || return 0 ;&
	sys_info)
		run "printf '%s\n' sys_info-${SYS_INFO_VERSION}"
		$all || return 0 ;&
	tcpdump)
		if which_prog tcpdumppath tcpdump; then
			run "${tcpdumppath} -h 2>&1 | awk '\$1 == \"tcpdump\" { sub(\" version \", \"-\"); print }'"
		fi
		$all || return 0 ;&
	tcsh)
		if which_prog tcshpath tcsh; then
			run "${tcshpath} --version | awk '{ print \$1 \"-\" \$2 }'"
		fi
		$all || return 0 ;&
	tzdata)
		if [ -f ${destdir}/usr/share/zoneinfo/TZDATA_VERSION ]; then
			run "cat ${destdir}/usr/share/zoneinfo/TZDATA_VERSION"
		else
			run "printf '%s\n' tzdata-too-old-to-matter"
		fi
		$all || return 0 ;&
	unbound)
		if which_prog unboundpath unbound-control; then
			run "${unboundpath} -h | awk '/^Version/ { print \"unbound-\" \$2 }'"
		else
			$all || printf >&2 '%s\n' "unbound: not found"
		fi
		$all || return 0 ;&
	[uU]ser[lL]and|release)
		run "sed <${destdir}/etc/release -e 's/ /-/g' -e 's/^/userland-/' -e 1q"
		$all || return 0 ;&
	wpa_supplicant)
		if which_prog wpapath wpa_supplicant; then
			run "${wpapath} -v | awk 'NF == 2 { sub(\" v\", \"-\"); print }'"
		fi
		$all || return 0 ;&
	xz)
		run "xz --version | awk '{ print \$1 \"-\" \$4; exit }'"
		$all || return 0 ;&
	yacc)
		run "yacc -V | sed -e 's| ||g'"
		$all || return 0 ;&

	'')			# never matches
		;;		# but terminates ;& sequence

	*)	printf >&2 '%s\n' "Unrecognised subsystem: $1"
		ERRS=1
		;;
	esac
}

verbose=false
destdir=""
# check if we have our only option
while getopts "L:P:d:v" a; do
	case "$a" in
	L)	LIBRARY_PATH=${OPTARG};;
	P)	PATH=${OPTARG};;
	d)	PATH=${OPTARG}/bin:${OPTARG}/sbin:${OPTARG}/usr/bin:${OPTARG}/usr/sbin
		LIBRARY_PATH=${OPTARG}/usr/lib:${OPTARG}/usr.X11R7/lib
		destdir=${OPTARG};;
	v)	verbose=true;;
	\?)	printf >&2 '%s\n' \
		    "Usage: $0 [-v] [-d destdir] [-L libdirs] [-P path] [system...]"
		exit 2
	esac
done
shift $((OPTIND - 1))

if [ $# -eq 0 ]; then
	set -- ''
	all=true
else
	all=false
fi

ERRS=0
while [ $# -gt 0 ]; do
	getversion "$1"
	shift
done
exit $ERRS
