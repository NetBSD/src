#! /usr/bin/env sh
#	$NetBSD: build.sh,v 1.184 2008/02/03 06:10:53 matt Exp $
#
# Copyright (c) 2001-2005 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Todd Vierling and Luke Mewburn.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# Top level build wrapper, for a system containing no tools.
#
# This script should run on any POSIX-compliant shell.  If the
# first "sh" found in the PATH is a POSIX-compliant shell, then
# you should not need to take any special action.  Otherwise, you
# should set the environment variable HOST_SH to a POSIX-compliant
# shell, and invoke build.sh with that shell.  (Depending on your
# system, one of /bin/ksh, /usr/local/bin/bash, or /usr/xpg4/bin/sh
# might be a suitable shell.)
#

progname=${0##*/}
toppid=$$
results=/dev/null
trap "exit 1" 1 2 3 15

bomb()
{
	cat >&2 <<ERRORMESSAGE

ERROR: $@
*** BUILD ABORTED ***
ERRORMESSAGE
	kill ${toppid}		# in case we were invoked from a subshell
	exit 1
}


statusmsg()
{
	${runcmd} echo "===> $@" | tee -a "${results}"
}

warning()
{
	statusmsg "Warning: $@"
}

# Find a program in the PATH, and print the result.  If not found,
# print a default.  If $2 is defined (even if it is an empty string),
# then that is the default; otherwise, $1 is used as the default.
find_in_PATH()
{
	local prog="$1"
	local result="${2-"$1"}"
	local oldIFS="${IFS}"
	local dir
	IFS=":"
	for dir in ${PATH}; do
		if [ -x "${dir}/${prog}" ]; then
			result="${dir}/${prog}"
			break
		fi
	done
	IFS="${oldIFS}"
	echo "${result}"
}

# Try to find a working POSIX shell, and set HOST_SH to refer to it.
# Assumes that uname_s, uname_m, and PWD have been set.
set_HOST_SH()
{
	# Even if ${HOST_SH} is already defined, we still do the
	# sanity checks at the end.

	# Solaris has /usr/xpg4/bin/sh.
	#
	[ -z "${HOST_SH}" ] && [ x"${uname_s}" = x"SunOS" ] && \
		[ -x /usr/xpg4/bin/sh ] && HOST_SH="/usr/xpg4/bin/sh"

	# Try to get the name of the shell that's running this script,
	# by parsing the output from "ps".  We assume that, if the host
	# system's ps command supports -o comm at all, it will do so
	# in the usual way: a one-line header followed by a one-line
	# result, possibly including trailing white space.  And if the
	# host system's ps command doesn't support -o comm, we assume
	# that we'll get an error message on stderr and nothing on
	# stdout.  (We don't try to use ps -o 'comm=' to suppress the
	# header line, because that is less widely supported.)
	#
	# If we get the wrong result here, the user can override it by
	# specifying HOST_SH in the environment.
	#
	[ -z "${HOST_SH}" ] && HOST_SH="$(
		(ps -p $$ -o comm | sed -ne '2s/[ \t]*$//p') 2>/dev/null )"

	# If nothing above worked, use "sh".  We will later find the
	# first directory in the PATH that has a "sh" program.
	#
	[ -z "${HOST_SH}" ] && HOST_SH="sh"

	# If the result so far is not an absolute path, try to prepend
	# PWD or search the PATH.
	#
	case "${HOST_SH}" in
	/*)	:
		;;
	*/*)	HOST_SH="${PWD}/${HOST_SH}"
		;;
	*)	HOST_SH="$(find_in_PATH "${HOST_SH}")"
		;;
	esac

	# If we don't have an absolute path by now, bomb.
	#
	case "${HOST_SH}" in
	/*)	:
		;;
	*)	bomb "HOST_SH=\"${HOST_SH}\" is not an absolute path."
		;;
	esac

	# If HOST_SH is not executable, bomb.
	#
	[ -x "${HOST_SH}" ] ||
	    bomb "HOST_SH=\"${HOST_SH}\" is not executable."
}

initdefaults()
{
	makeenv=
	makewrapper=
	makewrappermachine=
	runcmd=
	operations=
	removedirs=

	[ -d usr.bin/make ] || cd "$(dirname $0)"
	[ -d usr.bin/make ] ||
	    bomb "build.sh must be run from the top source level"
	[ -f share/mk/bsd.own.mk ] ||
	    bomb "src/share/mk is missing; please re-fetch the source tree"

	# Find information about the build platform.  Note that "uname -p"
	# is not part of POSIX, but NetBSD's uname -p prints MACHINE_ARCH,
	# while uname -m prints MACHINE.
	#
	uname_s=$(uname -s 2>/dev/null)
	uname_r=$(uname -r 2>/dev/null)
	uname_m=$(uname -m 2>/dev/null)
	uname_p=$(uname -p 2>/dev/null || uname -m 2>/dev/null)

	# If $PWD is a valid name of the current directory, POSIX mandates
	# that pwd return it by default which causes problems in the
	# presence of symlinks.  Unsetting PWD is simpler than changing
	# every occurrence of pwd to use -P.
	#
	# XXX Except that doesn't work on Solaris. Or many Linuces.
	#
	unset PWD
	TOP=$(/bin/pwd -P 2>/dev/null || /bin/pwd 2>/dev/null)

	# The user can set HOST_SH in the environment, or we try to
	# guess an appropriate value.  Then we set several other
	# variables from HOST_SH.
	#
	set_HOST_SH
	setmakeenv HOST_SH "${HOST_SH}"
	setmakeenv BSHELL "${HOST_SH}"
	setmakeenv CONFIG_SHELL "${HOST_SH}"

	# Set defaults.
	#
	toolprefix=nb

	# Some systems have a small ARG_MAX.  -X prevents make(1) from
	# exporting variables in the environment redundantly.
	#
	case "${uname_s}" in
	Darwin | FreeBSD | CYGWIN*)
		MAKEFLAGS=-X
		;;
	*)
		MAKEFLAGS=
		;;
	esac

	# do_{operation}=true if given operation is requested.
	#
	do_expertmode=false
	do_rebuildmake=false
	do_removedirs=false
	do_tools=false
	do_obj=false
	do_build=false
	do_distribution=false
	do_release=false
	do_kernel=false
	do_releasekernel=false
	do_install=false
	do_sets=false
	do_sourcesets=false
	do_syspkgs=false
	do_iso_image=false
	do_iso_image_source=false
	do_params=false

	# Create scratch directory
	#
	tmpdir="${TMPDIR-/tmp}/nbbuild$$"
	mkdir "${tmpdir}" || bomb "Cannot mkdir: ${tmpdir}"
	trap "cd /; rm -r -f \"${tmpdir}\"" 0
	results="${tmpdir}/build.sh.results"

	# Set source directories
	#
	setmakeenv NETBSDSRCDIR "${TOP}"

	# Find the version of NetBSD
	#
	DISTRIBVER="$(${HOST_SH} ${TOP}/sys/conf/osrelease.sh)"

	# Set various environment variables to known defaults,
	# to minimize (cross-)build problems observed "in the field".
	#
	unsetmakeenv INFODIR
	unsetmakeenv LESSCHARSET
	setmakeenv LC_ALL C
}

getarch()
{
	# Translate some MACHINE name aliases (known only to build.sh)
	# into proper MACHINE and MACHINE_ARCH names.  Save the alias
	# name in makewrappermachine.
	#
	case "${MACHINE}" in

	evbarm-e[bl])
		makewrappermachine=${MACHINE}
		# MACHINE_ARCH is "arm" or "armeb", not "armel"
		MACHINE_ARCH=arm${MACHINE##*-}
		MACHINE_ARCH=${MACHINE_ARCH%el}
		MACHINE=${MACHINE%-e[bl]}
		;;

	evbmips-e[bl]|sbmips-e[bl])
		makewrappermachine=${MACHINE}
		MACHINE_ARCH=mips${MACHINE##*-}
		MACHINE=${MACHINE%-e[bl]}
		;;

	evbmips64-e[bl]|sbmips64-e[bl])
		makewrappermachine=${MACHINE}
		MACHINE_ARCH=mips64${MACHINE##*-}
		MACHINE=${MACHINE%64-e[bl]}
		;;

	evbsh3-e[bl])
		makewrappermachine=${MACHINE}
		MACHINE_ARCH=sh3${MACHINE##*-}
		MACHINE=${MACHINE%-e[bl]}
		;;

	esac

	# Translate a MACHINE into a default MACHINE_ARCH.
	#
	case "${MACHINE}" in

	acorn26|acorn32|cats|hpcarm|iyonix|netwinder|shark|zaurus)
		MACHINE_ARCH=arm
		;;

	evbarm)		# unspecified MACHINE_ARCH gets LE
		MACHINE_ARCH=${MACHINE_ARCH:=arm}
		;;

	hp700)
		MACHINE_ARCH=hppa
		;;

	sun2)
		MACHINE_ARCH=m68000
		;;

	amiga|atari|cesfic|hp300|luna68k|mac68k|mvme68k|news68k|next68k|sun3|x68k)
		MACHINE_ARCH=m68k
		;;

	evbmips|sbmips)		# no default MACHINE_ARCH
		;;

	ews4800mips|mipsco|newsmips|sgimips)
		MACHINE_ARCH=mipseb
		;;

	algor|arc|cobalt|hpcmips|playstation2|pmax)
		MACHINE_ARCH=mipsel
		;;

	evbppc64|macppc64|ofppc64)
		makewrappermachine=${MACHINE}
		MACHINE=${MACHINE%64}
		MACHINE_ARCH=powerpc64
		;;

	amigappc|bebox|evbppc|ibmnws|macppc|mvmeppc|ofppc|prep|rs6000|sandpoint)
		MACHINE_ARCH=powerpc
		;;

	evbsh3)			# no default MACHINE_ARCH
		;;

	mmeye)
		MACHINE_ARCH=sh3eb
		;;

	dreamcast|hpcsh|landisk)
		MACHINE_ARCH=sh3el
		;;

	amd64)
		MACHINE_ARCH=x86_64
		;;

	alpha|i386|sparc|sparc64|vax|ia64)
		MACHINE_ARCH=${MACHINE}
		;;

	*)
		bomb "Unknown target MACHINE: ${MACHINE}"
		;;

	esac
}

validatearch()
{
	# Ensure that the MACHINE_ARCH exists (and is supported by build.sh).
	#
	case "${MACHINE_ARCH}" in

	alpha|arm|armeb|hppa|i386|m68000|m68k|mipse[bl]|mips64e[bl]|powerpc|powerpc64|sh3e[bl]|sparc|sparc64|vax|x86_64|ia64)
		;;

	"")
		bomb "No MACHINE_ARCH provided"
		;;

	*)
		bomb "Unknown target MACHINE_ARCH: ${MACHINE_ARCH}"
		;;

	esac

	# Determine valid MACHINE_ARCHs for MACHINE
	#
	case "${MACHINE}" in

	evbarm)
		arches="arm armeb"
		;;

	evbmips|sbmips)
		arches="mipseb mipsel mips64eb mips64el"
		;;

	sgimips)
		arches="mipseb mips64eb"
		;;

	evbsh3)
		arches="sh3eb sh3el"
		;;

	macppc|evbppc|ofppc)
		arches="powerpc powerpc64"
		;;
	*)
		oma="${MACHINE_ARCH}"
		getarch
		arches="${MACHINE_ARCH}"
		MACHINE_ARCH="${oma}"
		;;

	esac

	# Ensure that MACHINE_ARCH supports MACHINE
	#
	archok=false
	for a in ${arches}; do
		if [ "${a}" = "${MACHINE_ARCH}" ]; then
			archok=true
			break
		fi
	done
	${archok} ||
	    bomb "MACHINE_ARCH '${MACHINE_ARCH}' does not support MACHINE '${MACHINE}'"
}

nobomb_getmakevar()
{
	[ -x "${make}" ] || return 1
	"${make}" -m ${TOP}/share/mk -s -B -f- _x_ <<EOF || return 1
_x_:
	echo \${$1}
.include <bsd.prog.mk>
.include <bsd.kernobj.mk>
EOF
}

raw_getmakevar()
{
	[ -x "${make}" ] || bomb "raw_getmakevar $1: ${make} is not executable"
	nobomb_getmakevar "$1" || bomb "raw_getmakevar $1: ${make} failed"
}

getmakevar()
{
	# raw_getmakevar() doesn't work properly if $make hasn't yet been
	# built, which can happen when running with the "-n" option.
	# getmakevar() deals with this by emitting a literal '$'
	# followed by the variable name, instead of trying to find the
	# variable's value.
	#
	if [ -x "${make}" ]; then
		raw_getmakevar "$1"
	else
		echo "\$$1"
	fi
}

setmakeenv()
{
	eval "$1='$2'; export $1"
	makeenv="${makeenv} $1"
}

unsetmakeenv()
{
	eval "unset $1"
	makeenv="${makeenv} $1"
}

# Convert possibly-relative path to absolute path by prepending
# ${TOP} if necessary.  Also delete trailing "/", if any.
resolvepath()
{
	case "${OPTARG}" in
	/)
		;;
	/*)
		OPTARG="${OPTARG%/}"
		;;
	*)
		OPTARG="${TOP}/${OPTARG%/}"
		;;
	esac
}

usage()
{
	if [ -n "$*" ]; then
		echo ""
		echo "${progname}: $*"
	fi
	cat <<_usage_

Usage: ${progname} [-EnorUux] [-a arch] [-B buildid] [-C cdextras] [-D dest]
		[-j njob] [-M obj] [-m mach] [-N noisy] [-O obj] [-R release]
		[-T tools] [-V var=[value]] [-w wrapper] [-X x11src] [-Z var]
		operation [...]

 Build operations (all imply "obj" and "tools"):
    build               Run "make build".
    distribution        Run "make distribution" (includes DESTDIR/etc/ files).
    release             Run "make release" (includes kernels & distrib media).

 Other operations:
    help                Show this message and exit.
    makewrapper         Create ${toolprefix}make-\${MACHINE} wrapper and ${toolprefix}make.
                        Always performed.
    obj                 Run "make obj".  [Default unless -o is used]
    tools               Build and install tools.
    install=idir        Run "make installworld" to \`idir' to install all sets
			except \`etc'.  Useful after "distribution" or "release"
    kernel=conf         Build kernel with config file \`conf'
    releasekernel=conf  Install kernel built by kernel=conf to RELEASEDIR.
    sets                Create binary sets in RELEASEDIR/MACHINE/binary/sets.
			DESTDIR should be populated beforehand.
    sourcesets          Create source sets in RELEASEDIR/source/sets.
    syspkgs             Create syspkgs in RELEASEDIR/MACHINE/binary/syspkgs.
    iso-image           Create CD-ROM image in RELEASEDIR/iso.
    iso-image-source    Create CD-ROM image with source in RELEASEDIR/iso.
    params              Display various make(1) parameters.

 Options:
    -a arch     Set MACHINE_ARCH to arch.  [Default: deduced from MACHINE]
    -B buildId  Set BUILDID to buildId.
    -C cdextras Set CDEXTRA to cdextras
    -D dest     Set DESTDIR to dest.  [Default: destdir.MACHINE]
    -E          Set "expert" mode; disables various safety checks.
                Should not be used without expert knowledge of the build system.
    -h          Print this help message.
    -j njob     Run up to njob jobs in parallel; see make(1) -j.
    -M obj      Set obj root directory to obj; sets MAKEOBJDIRPREFIX.
                Unsets MAKEOBJDIR.
    -m mach     Set MACHINE to mach; not required if NetBSD native.
    -N noisy	Set the noisyness (MAKEVERBOSE) level of the build:
		    0	Quiet
		    1	Operations are described, commands are suppressed
		    2	Full output
		[Default: 2]
    -n          Show commands that would be executed, but do not execute them.
    -O obj      Set obj root directory to obj; sets a MAKEOBJDIR pattern.
                Unsets MAKEOBJDIRPREFIX.
    -o          Set MKOBJDIRS=no; do not create objdirs at start of build.
    -R release  Set RELEASEDIR to release.  [Default: releasedir]
    -r          Remove contents of TOOLDIR and DESTDIR before building.
    -T tools    Set TOOLDIR to tools.  If unset, and TOOLDIR is not set in
                the environment, ${toolprefix}make will be (re)built unconditionally.
    -U          Set MKUNPRIVED=yes; build without requiring root privileges,
    		install from an UNPRIVED build with proper file permissions.
    -u          Set MKUPDATE=yes; do not run "make clean" first.
		Without this, everything is rebuilt, including the tools.
    -V v=[val]  Set variable \`v' to \`val'.
    -w wrapper  Create ${toolprefix}make script as wrapper.
                [Default: \${TOOLDIR}/bin/${toolprefix}make-\${MACHINE}]
    -X x11src   Set X11SRCDIR to x11src.  [Default: /usr/xsrc]
    -x          Set MKX11=yes; build X11R6 from X11SRCDIR
    -Z v        Unset ("zap") variable \`v'.

_usage_
	exit 1
}

parseoptions()
{
	opts='a:B:bC:D:dEhi:j:k:M:m:N:nO:oR:rT:tUuV:w:xX:Z:'
	opt_a=no

	if type getopts >/dev/null 2>&1; then
		# Use POSIX getopts.
		#
		getoptcmd='getopts ${opts} opt && opt=-${opt}'
		optargcmd=':'
		optremcmd='shift $((${OPTIND} -1))'
	else
		type getopt >/dev/null 2>&1 ||
		    bomb "/bin/sh shell is too old; try ksh or bash"

		# Use old-style getopt(1) (doesn't handle whitespace in args).
		#
		args="$(getopt ${opts} $*)"
		[ $? = 0 ] || usage
		set -- ${args}

		getoptcmd='[ $# -gt 0 ] && opt="$1" && shift'
		optargcmd='OPTARG="$1"; shift'
		optremcmd=':'
	fi

	# Parse command line options.
	#
	while eval ${getoptcmd}; do
		case ${opt} in

		-a)
			eval ${optargcmd}
			MACHINE_ARCH=${OPTARG}
			opt_a=yes
			;;

		-B)
			eval ${optargcmd}
			BUILDID=${OPTARG}
			;;

		-b)
			usage "'-b' has been replaced by 'makewrapper'"
			;;

		-C)
			eval ${optargcmd}; resolvepath
			iso_dir=${OPTARG}
			;;

		-D)
			eval ${optargcmd}; resolvepath
			setmakeenv DESTDIR "${OPTARG}"
			;;

		-d)
			usage "'-d' has been replaced by 'distribution'"
			;;

		-E)
			do_expertmode=true
			;;

		-i)
			usage "'-i idir' has been replaced by 'install=idir'"
			;;

		-j)
			eval ${optargcmd}
			parallel="-j ${OPTARG}"
			;;

		-k)
			usage "'-k conf' has been replaced by 'kernel=conf'"
			;;

		-M)
			eval ${optargcmd}; resolvepath
			makeobjdir="${OPTARG}"
			unsetmakeenv MAKEOBJDIR
			setmakeenv MAKEOBJDIRPREFIX "${OPTARG}"
			;;

			# -m overrides MACHINE_ARCH unless "-a" is specified
		-m)
			eval ${optargcmd}
			MACHINE="${OPTARG}"
			[ "${opt_a}" != "yes" ] && getarch
			;;

		-N)
			eval ${optargcmd}
			case "${OPTARG}" in
			0|1|2)
				setmakeenv MAKEVERBOSE "${OPTARG}"
				;;
			*)
				usage "'${OPTARG}' is not a valid value for -N"
				;;
			esac
			;;

		-n)
			runcmd=echo
			;;

		-O)
			eval ${optargcmd}; resolvepath
			makeobjdir="${OPTARG}"
			unsetmakeenv MAKEOBJDIRPREFIX
			setmakeenv MAKEOBJDIR "\${.CURDIR:C,^$TOP,$OPTARG,}"
			;;

		-o)
			MKOBJDIRS=no
			;;

		-R)
			eval ${optargcmd}; resolvepath
			setmakeenv RELEASEDIR "${OPTARG}"
			;;

		-r)
			do_removedirs=true
			do_rebuildmake=true
			;;

		-T)
			eval ${optargcmd}; resolvepath
			TOOLDIR="${OPTARG}"
			export TOOLDIR
			;;

		-t)
			usage "'-t' has been replaced by 'tools'"
			;;

		-U)
			setmakeenv MKUNPRIVED yes
			;;

		-u)
			setmakeenv MKUPDATE yes
			;;

		-V)
			eval ${optargcmd}
			case "${OPTARG}" in
		    # XXX: consider restricting which variables can be changed?
			[a-zA-Z_][a-zA-Z_0-9]*=*)
				setmakeenv "${OPTARG%%=*}" "${OPTARG#*=}"
				;;
			*)
				usage "-V argument must be of the form 'var=[value]'"
				;;
			esac
			;;

		-w)
			eval ${optargcmd}; resolvepath
			makewrapper="${OPTARG}"
			;;

		-X)
			eval ${optargcmd}; resolvepath
			setmakeenv X11SRCDIR "${OPTARG}"
			;;

		-x)
			setmakeenv MKX11 yes
			;;

		-Z)
			eval ${optargcmd}
		    # XXX: consider restricting which variables can be unset?
			unsetmakeenv "${OPTARG}"
			;;

		--)
			break
			;;

		-'?'|-h)
			usage
			;;

		esac
	done

	# Validate operations.
	#
	eval ${optremcmd}
	while [ $# -gt 0 ]; do
		op=$1; shift
		operations="${operations} ${op}"

		case "${op}" in

		help)
			usage
			;;

		makewrapper|obj|tools|build|distribution|release|sets|sourcesets|syspkgs|params)
			;;

		iso-image)
			op=iso_image	# used as part of a variable name
			;;

		iso-image-source)
			op=iso_image_source   # used as part of a variable name
			;;

		kernel=*|releasekernel=*)
			arg=${op#*=}
			op=${op%%=*}
			[ -n "${arg}" ] ||
			    bomb "Must supply a kernel name with \`${op}=...'"
			;;

		install=*)
			arg=${op#*=}
			op=${op%%=*}
			[ -n "${arg}" ] ||
			    bomb "Must supply a directory with \`install=...'"
			;;

		*)
			usage "Unknown operation \`${op}'"
			;;

		esac
		eval do_${op}=true
	done
	[ -n "${operations}" ] || usage "Missing operation to perform."

	# Set up MACHINE*.  On a NetBSD host, these are allowed to be unset.
	#
	if [ -z "${MACHINE}" ]; then
		[ "${uname_s}" = "NetBSD" ] ||
		    bomb "MACHINE must be set, or -m must be used, for cross builds."
		MACHINE=${uname_m}
	fi
	[ -n "${MACHINE_ARCH}" ] || getarch
	validatearch

	# Set up default make(1) environment.
	#
	makeenv="${makeenv} TOOLDIR MACHINE MACHINE_ARCH MAKEFLAGS"
	[ -z "${BUILDID}" ] || makeenv="${makeenv} BUILDID"
	MAKEFLAGS="-de -m ${TOP}/share/mk ${MAKEFLAGS} MKOBJDIRS=${MKOBJDIRS-yes}"
	export MAKEFLAGS MACHINE MACHINE_ARCH
}

sanitycheck()
{
	# If the PATH contains any non-absolute components (including,
	# but not limited to, "." or ""), then complain.  As an exception,
	# allow "" or "." as the last component of the PATH.  This is fatal
	# if expert mode is not in effect.
	#
	local path="${PATH}"
	path="${path%:}"	# delete trailing ":"
	path="${path%:.}"	# delete trailing ":."
	case ":${path}:/" in
	*:[!/]*)
		if ${do_expertmode}; then
			warning "PATH contains non-absolute components"
		else
			bomb "PATH environment variable must not" \
			     "contain non-absolute components"
		fi
		;;
	esac
}

# Try to set a value for TOOLDIR.  This is difficult because of a cyclic
# dependency: TOOLDIR may be affected by settings in /etc/mk.conf, so
# we would like to use getmakevar to get the value of TOOLDIR, but we
# can't use getmakevar before we have an up to date version of nbmake;
# we might already have an up to date version of nbmake in TOOLDIR, but
# we don't yet know where TOOLDIR is.
#
# In principle, we could break the cycle by building a copy of nbmake
# in a temporary directory.  However, people who use the default value
# of TOOLDIR do not like to have nbmake rebuilt every time they run
# build.sh.
#
# We try to please everybody as follows:
#
# * If TOOLDIR was set in the environment or on the command line, use
#   that value.
# * Otherwise try to guess what TOOLDIR would be if not overridden by
#   /etc/mk.conf, and check whether the resulting directory contains
#   a copy of ${toolprefix}make (this should work for everybody who
#   doesn't override TOOLDIR via /etc/mk.conf);
# * Failing that, search for ${toolprefix}make, nbmake, bmake, or make,
#   in the PATH (this might accidentally find a non-NetBSD version of
#   make, which will lead to failure in the next step);
# * If a copy of make was found above, try to use it with
#   nobomb_getmakevar to find the correct value for TOOLDIR;
# * If all else fails, leave TOOLDIR unset.  Our caller is expected to
#   be able to cope with this.
#
try_set_TOOLDIR()
{
	[ -n "${TOOLDIR}" ] && return

	# Set guess_TOOLDIR, in the same way that <bsd.own.mk> would set
	# TOOLDIR if /etc/mk.conf sisn't interfere.
	local topobjdir="${TOP}"
	[ -n "${makeobjdir}" ] && topobjdir="${topobjdir}/${makeobjdir}"
	local host_ostype="${uname_s}-$(
		echo "${uname_r}" | sed -e 's/([^)]*)//g' -e 's/ /_/g'
		)$(
		echo "${uname_p}" | sed -e 's/([^)]*)//g' -e 's/ /_/g'
		)"
	local guess_TOOLDIR="${topobjdir}/tooldir.${host_ostype}"

	# Look for a suitable ${toolprefix}make, nbmake, bmake, or make.
	guess_make="${guess_TOOLDIR}/bin/${toolprefix}make"
	[ -x "${guess_make}" ] || guess_make=""
	: ${guess_make:=$(find_in_PATH ${toolprefix}make '')}
	: ${guess_make:=$(find_in_PATH nbmake '')}
	: ${guess_make:=$(find_in_PATH bmake '')}
	: ${guess_make:=$(find_in_PATH make '')}

	# Use ${guess_make} with nobomb_getmakevar
	if [ -x "${guess_make}" ]; then
		TOOLDIR=$(make="${guess_make}" nobomb_getmakevar TOOLDIR)
		[ -n "${TOOLDIR}" ] || unset TOOLDIR
	fi
}

rebuildmake()
{
	# Test make source file timestamps against installed ${toolprefix}make
	# binary, if TOOLDIR is pre-set or if try_set_TOOLDIR can set it.
	#
	try_set_TOOLDIR
	make="${TOOLDIR-nonexistent}/bin/${toolprefix}make"
	if [ -x "${make}" ]; then
		for f in usr.bin/make/*.[ch] usr.bin/make/lst.lib/*.[ch]; do
			if [ "${f}" -nt "${make}" ]; then
				statusmsg "${make} outdated (older than ${f}), needs building."
				do_rebuildmake=true
				break
			fi
		done
	else
		statusmsg "No ${make}, needs building."
		do_rebuildmake=true
	fi

	# Build bootstrap ${toolprefix}make if needed.
	if ${do_rebuildmake}; then
		statusmsg "Bootstrapping ${toolprefix}make"
		${runcmd} cd "${tmpdir}"
		${runcmd} env CC="${HOST_CC-cc}" CPPFLAGS="${HOST_CPPFLAGS}" \
			CFLAGS="${HOST_CFLAGS--O}" LDFLAGS="${HOST_LDFLAGS}" \
			${HOST_SH} "${TOP}/tools/make/configure" ||
		    bomb "Configure of ${toolprefix}make failed"
		${runcmd} ${HOST_SH} buildmake.sh ||
		    bomb "Build of ${toolprefix}make failed"
		make="${tmpdir}/${toolprefix}make"
		${runcmd} cd "${TOP}"
		${runcmd} rm -f usr.bin/make/*.o usr.bin/make/lst.lib/*.o
	fi
}

validatemakeparams()
{
	if [ "${runcmd}" = "echo" ]; then
		TOOLCHAIN_MISSING=no
		EXTERNAL_TOOLCHAIN=""
	else
		TOOLCHAIN_MISSING=$(raw_getmakevar TOOLCHAIN_MISSING)
		EXTERNAL_TOOLCHAIN=$(raw_getmakevar EXTERNAL_TOOLCHAIN)
	fi
	if [ "${TOOLCHAIN_MISSING}" = "yes" ] && \
	   [ -z "${EXTERNAL_TOOLCHAIN}" ]; then
		${runcmd} echo "ERROR: build.sh (in-tree cross-toolchain) is not yet available for"
		${runcmd} echo "	MACHINE:      ${MACHINE}"
		${runcmd} echo "	MACHINE_ARCH: ${MACHINE_ARCH}"
		${runcmd} echo ""
		${runcmd} echo "All builds for this platform should be done via a traditional make"
		${runcmd} echo "If you wish to use an external cross-toolchain, set"
		${runcmd} echo "	EXTERNAL_TOOLCHAIN=<path to toolchain root>"
		${runcmd} echo "in either the environment or mk.conf and rerun"
		${runcmd} echo "	${progname} $*"
		exit 1
	fi

	# Normalise MKOBJDIRS, MKUNPRIVED, and MKUPDATE
	# These may be set as build.sh options or in "mk.conf".
	# Don't export them as they're only used for tests in build.sh.
	#
	MKOBJDIRS=$(getmakevar MKOBJDIRS)
	MKUNPRIVED=$(getmakevar MKUNPRIVED)
	MKUPDATE=$(getmakevar MKUPDATE)

	if [ "${MKOBJDIRS}" != "no" ]; then
		# If setting -M or -O to the root of an obj dir, make sure
		# the base directory is made before continuing as <bsd.own.mk>
		# will need this to pick up _SRC_TOP_OBJ_
		#
		if [ ! -z "${makeobjdir}" ]; then
			${runcmd} mkdir -p "${makeobjdir}"
		fi

		# make obj in tools to ensure that the objdir for the top-level
		# of the source tree and for "tools" is available, in case the
		# default TOOLDIR setting from <bsd.own.mk> is used, or the
		# build.sh default DESTDIR and RELEASEDIR is to be used.
		#
		${runcmd} cd tools
		${runcmd} "${make}" -m ${TOP}/share/mk obj NOSUBDIR= ||
		    bomb "Failed to make obj in tools"
		${runcmd} cd "${TOP}"
	fi

	# Find TOOLDIR, DESTDIR, and RELEASEDIR.
	#
	TOOLDIR=$(getmakevar TOOLDIR)
	statusmsg "TOOLDIR path:     ${TOOLDIR}"
	DESTDIR=$(getmakevar DESTDIR)
	RELEASEDIR=$(getmakevar RELEASEDIR)
	if ! $do_expertmode; then
		_SRC_TOP_OBJ_=$(getmakevar _SRC_TOP_OBJ_)
		: ${DESTDIR:=${_SRC_TOP_OBJ_}/destdir.${MACHINE}}
		: ${RELEASEDIR:=${_SRC_TOP_OBJ_}/releasedir}
		makeenv="${makeenv} DESTDIR RELEASEDIR"
	fi
	export TOOLDIR DESTDIR RELEASEDIR
	statusmsg "DESTDIR path:     ${DESTDIR}"
	statusmsg "RELEASEDIR path:  ${RELEASEDIR}"

	# Check validity of TOOLDIR and DESTDIR.
	#
	if [ -z "${TOOLDIR}" ] || [ "${TOOLDIR}" = "/" ]; then
		bomb "TOOLDIR '${TOOLDIR}' invalid"
	fi
	removedirs="${TOOLDIR}"

	if [ -z "${DESTDIR}" ] || [ "${DESTDIR}" = "/" ]; then
		if ${do_build} || ${do_distribution} || ${do_release}; then
			if ! ${do_build} || \
			   [ "${uname_s}" != "NetBSD" ] || \
			   [ "${uname_m}" != "${MACHINE}" ]; then
				bomb "DESTDIR must != / for cross builds, or ${progname} 'distribution' or 'release'."
			fi
			if ! ${do_expertmode}; then
				bomb "DESTDIR must != / for non -E (expert) builds"
			fi
			statusmsg "WARNING: Building to /, in expert mode."
			statusmsg "         This may cause your system to break!  Reasons include:"
			statusmsg "            - your kernel is not up to date"
			statusmsg "            - the libraries or toolchain have changed"
			statusmsg "         YOU HAVE BEEN WARNED!"
		fi
	else
		removedirs="${removedirs} ${DESTDIR}"
	fi
	if ${do_build} || ${do_distribution} || ${do_release}; then
		if ! ${do_expertmode} && \
		    [ "$(id -u 2>/dev/null)" -ne 0 ] && \
		    [ "${MKUNPRIVED}" = "no" ] ; then
			bomb "-U or -E must be set for build as an unprivileged user."
		fi
        fi
	if ${do_releasekernel} && [ -z "${RELEASEDIR}" ]; then
		bomb "Must set RELEASEDIR with \`releasekernel=...'"
	fi
}


createmakewrapper()
{
	# Remove the target directories.
	#
	if ${do_removedirs}; then
		for f in ${removedirs}; do
			statusmsg "Removing ${f}"
			${runcmd} rm -r -f "${f}"
		done
	fi

	# Recreate $TOOLDIR.
	#
	${runcmd} mkdir -p "${TOOLDIR}/bin" ||
	    bomb "mkdir of '${TOOLDIR}/bin' failed"

	# Install ${toolprefix}make if it was built.
	#
	if ${do_rebuildmake}; then
		${runcmd} rm -f "${TOOLDIR}/bin/${toolprefix}make"
		${runcmd} cp "${make}" "${TOOLDIR}/bin/${toolprefix}make" ||
		    bomb "Failed to install \$TOOLDIR/bin/${toolprefix}make"
		make="${TOOLDIR}/bin/${toolprefix}make"
		statusmsg "Created ${make}"
	fi

	# Build a ${toolprefix}make wrapper script, usable by hand as
	# well as by build.sh.
	#
	if [ -z "${makewrapper}" ]; then
		makewrapper="${TOOLDIR}/bin/${toolprefix}make-${makewrappermachine:-${MACHINE}}"
		[ -z "${BUILDID}" ] || makewrapper="${makewrapper}-${BUILDID}"
	fi

	${runcmd} rm -f "${makewrapper}"
	if [ "${runcmd}" = "echo" ]; then
		echo 'cat <<EOF >'${makewrapper}
		makewrapout=
	else
		makewrapout=">>\${makewrapper}"
	fi

	case "${KSH_VERSION:-${SH_VERSION}}" in
	*PD\ KSH*|*MIRBSD\ KSH*)
		set +o braceexpand
		;;
	esac

	eval cat <<EOF ${makewrapout}
#! ${HOST_SH}
# Set proper variables to allow easy "make" building of a NetBSD subtree.
# Generated from:  \$NetBSD: build.sh,v 1.184 2008/02/03 06:10:53 matt Exp $
# with these arguments: ${_args}
#

EOF
	{
		for f in ${makeenv}; do
			if eval "[ -z \"\${$f}\" -a \"\${${f}-X}\" = \"X\" ]"; then
				eval echo "unset ${f}"
			else
				eval echo "${f}=\'\$$(echo ${f})\'\;\ export\ ${f}"
			fi
		done

		eval cat <<EOF
MAKEWRAPPERMACHINE=${makewrappermachine:-${MACHINE}}; export MAKEWRAPPERMACHINE
USETOOLS=yes; export USETOOLS
EOF
	} | eval sort -u "${makewrapout}"
	eval cat <<EOF "${makewrapout}"

exec "\${TOOLDIR}/bin/${toolprefix}make" \${1+"\$@"}
EOF
	[ "${runcmd}" = "echo" ] && echo EOF
	${runcmd} chmod +x "${makewrapper}"
	statusmsg "makewrapper:      ${makewrapper}"
	statusmsg "Updated ${makewrapper}"
}

buildtools()
{
	if [ "${MKOBJDIRS}" != "no" ]; then
		${runcmd} "${makewrapper}" ${parallel} obj-tools ||
		    bomb "Failed to make obj-tools"
	fi
	${runcmd} cd tools
	if [ "${MKUPDATE}" = "no" ]; then
		${runcmd} "${makewrapper}" ${parallel} cleandir ||
		    bomb "Failed to make cleandir tools"
	fi
	${runcmd} "${makewrapper}" ${parallel} dependall ||
	    bomb "Failed to make dependall tools"
	${runcmd} "${makewrapper}" ${parallel} install ||
	    bomb "Failed to make install tools"
	statusmsg "Tools built to ${TOOLDIR}"
	${runcmd} cd "${TOP}"
}

getkernelconf()
{
	kernelconf="$1"
	if [ "${MKOBJDIRS}" != "no" ]; then
		# The correct value of KERNOBJDIR might
		# depend on a prior "make obj" in
		# ${KERNSRCDIR}/${KERNARCHDIR}/compile.
		#
		KERNSRCDIR="$(getmakevar KERNSRCDIR)"
		KERNARCHDIR="$(getmakevar KERNARCHDIR)"
		${runcmd} cd "${KERNSRCDIR}/${KERNARCHDIR}/compile"
		${runcmd} "${makewrapper}" ${parallel} obj ||
		    bomb "Failed to make obj in ${KERNSRCDIR}/${KERNARCHDIR}/compile"
		${runcmd} cd "${TOP}"
	fi
	KERNCONFDIR="$(getmakevar KERNCONFDIR)"
	KERNOBJDIR="$(getmakevar KERNOBJDIR)"
	case "${kernelconf}" in
	*/*)
		kernelconfpath="${kernelconf}"
		kernelconfname="${kernelconf##*/}"
		;;
	*)
		kernelconfpath="${KERNCONFDIR}/${kernelconf}"
		kernelconfname="${kernelconf}"
		;;
	esac
	kernelbuildpath="${KERNOBJDIR}/${kernelconfname}"
}

buildkernel()
{
	if ! ${do_tools} && ! ${buildkernelwarned:-false}; then
		# Building tools every time we build a kernel is clearly
		# unnecessary.  We could try to figure out whether rebuilding
		# the tools is necessary this time, but it doesn't seem worth
		# the trouble.  Instead, we say it's the user's responsibility
		# to rebuild the tools if necessary.
		#
		statusmsg "Building kernel without building new tools"
		buildkernelwarned=true
	fi
	getkernelconf $1
	statusmsg "Building kernel:  ${kernelconf}"
	statusmsg "Build directory:  ${kernelbuildpath}"
	${runcmd} mkdir -p "${kernelbuildpath}" ||
	    bomb "Cannot mkdir: ${kernelbuildpath}"
	if [ "${MKUPDATE}" = "no" ]; then
		${runcmd} cd "${kernelbuildpath}"
		${runcmd} "${makewrapper}" ${parallel} cleandir ||
		    bomb "Failed to make cleandir in ${kernelbuildpath}"
		${runcmd} cd "${TOP}"
	fi
	[ -x "${TOOLDIR}/bin/${toolprefix}config" ] \
	|| bomb "${TOOLDIR}/bin/${toolprefix}config does not exist. You need to \"$0 tools\" first."
	${runcmd} "${TOOLDIR}/bin/${toolprefix}config" -b "${kernelbuildpath}" \
		-s "${TOP}/sys" "${kernelconfpath}" ||
	    bomb "${toolprefix}config failed for ${kernelconf}"
	${runcmd} cd "${kernelbuildpath}"
	${runcmd} "${makewrapper}" ${parallel} depend ||
	    bomb "Failed to make depend in ${kernelbuildpath}"
	${runcmd} "${makewrapper}" ${parallel} all ||
	    bomb "Failed to make all in ${kernelbuildpath}"
	${runcmd} cd "${TOP}"

	if [ "${runcmd}" != "echo" ]; then
		statusmsg "Kernels built from ${kernelconf}:"
		kernlist=$(awk '$1 == "config" { print $2 }' ${kernelconfpath})
		for kern in ${kernlist:-netbsd}; do
			[ -f "${kernelbuildpath}/${kern}" ] && \
			    echo "  ${kernelbuildpath}/${kern}"
		done | tee -a "${results}"
	fi
}

releasekernel()
{
	getkernelconf $1
	kernelreldir="${RELEASEDIR}/${MACHINE}/binary/kernel"
	${runcmd} mkdir -p "${kernelreldir}"
	kernlist=$(awk '$1 == "config" { print $2 }' ${kernelconfpath})
	for kern in ${kernlist:-netbsd}; do
		builtkern="${kernelbuildpath}/${kern}"
		[ -f "${builtkern}" ] || continue
		releasekern="${kernelreldir}/${kern}-${kernelconfname}.gz"
		statusmsg "Kernel copy:      ${releasekern}"
		${runcmd} gzip -c -9 < "${builtkern}" > "${releasekern}"
	done
}

installworld()
{
	dir="$1"
	${runcmd} "${makewrapper}" INSTALLWORLDDIR="${dir}" installworld ||
	    bomb "Failed to make installworld to ${dir}"
	statusmsg "Successful installworld to ${dir}"
}


main()
{
	initdefaults
	_args=$@
	parseoptions "$@"

	sanitycheck

	build_start=$(date)
	statusmsg "${progname} command: $0 $@"
	statusmsg "${progname} started: ${build_start}"
	statusmsg "NetBSD version:   ${DISTRIBVER}"
	statusmsg "MACHINE:          ${MACHINE}"
	statusmsg "MACHINE_ARCH:     ${MACHINE_ARCH}"
	statusmsg "Build platform:   ${uname_s} ${uname_r} ${uname_m}"
	statusmsg "HOST_SH:          ${HOST_SH}"

	rebuildmake
	validatemakeparams
	createmakewrapper

	# Perform the operations.
	#
	for op in ${operations}; do
		case "${op}" in

		makewrapper)
			# no-op
			;;

		tools)
			buildtools
			;;

		sets)
			statusmsg "Building sets from pre-populated ${DESTDIR}"
			${runcmd} "${makewrapper}" ${parallel} ${op} ||
			    bomb "Failed to make ${op}"
			statusmsg "Successful make ${op}"
			;;

		obj|build|distribution|release|sourcesets|syspkgs|params)
			${runcmd} "${makewrapper}" ${parallel} ${op} ||
			    bomb "Failed to make ${op}"
			statusmsg "Successful make ${op}"
			;;

		iso-image|iso-image-source)
			${runcmd} "${makewrapper}" ${parallel} \
			    CDEXTRA=$iso_dir ${op} ||
			    bomb "Failed to make ${op}"
			statusmsg "Successful make ${op}"
			;;

		kernel=*)
			arg=${op#*=}
			buildkernel "${arg}"
			;;

		releasekernel=*)
			arg=${op#*=}
			releasekernel "${arg}"
			;;

		install=*)
			arg=${op#*=}
			if [ "${arg}" = "/" ] && \
			    (	[ "${uname_s}" != "NetBSD" ] || \
				[ "${uname_m}" != "${MACHINE}" ] ); then
				bomb "'${op}' must != / for cross builds."
			fi
			installworld "${arg}"
			;;

		*)
			bomb "Unknown operation \`${op}'"
			;;

		esac
	done

	statusmsg "${progname} ended:   $(date)"
	if [ -s "${results}" ]; then
		echo "===> Summary of results:"
		sed -e 's/^===>//;s/^/	/' "${results}"
		echo "===> ."
	fi
}

main "$@"
