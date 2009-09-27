#! /usr/bin/env sh
#	$NetBSD: build.sh,v 1.213 2009/09/27 22:02:41 apb Exp $
#
# Copyright (c) 2001-2009 The NetBSD Foundation, Inc.
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
tab='	'
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
		(ps -p $$ -o comm | sed -ne "2s/[ ${tab}]*\$//p") 2>/dev/null )"

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

	# Find information about the build platform.  This should be
	# kept in sync with _HOST_OSNAME, _HOST_OSREL, and _HOST_ARCH
	# variables in share/mk/bsd.sys.mk.
	#
	# Note that "uname -p" is not part of POSIX, but we want uname_p
	# to be set to the host MACHINE_ARCH, if possible.  On systems
	# where "uname -p" fails, prints "unknown", or prints a string
	# that does not look like an identifier, fall back to using the
	# output from "uname -m" instead.
	#
	uname_s=$(uname -s 2>/dev/null)
	uname_r=$(uname -r 2>/dev/null)
	uname_m=$(uname -m 2>/dev/null)
	uname_p=$(uname -p 2>/dev/null || echo "unknown")
	case "${uname_p}" in
	''|unknown|*[^-_A-Za-z0-9]*) uname_p="${uname_m}" ;;
	esac

	id_u=$(id -u 2>/dev/null || /usr/xpg4/bin/id -u 2>/dev/null)

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
	do_cleandir=false
	do_obj=false
	do_build=false
	do_distribution=false
	do_release=false
	do_kernel=false
	do_releasekernel=false
	do_modules=false
	do_install=false
	do_sets=false
	do_sourcesets=false
	do_syspkgs=false
	do_iso_image=false
	do_iso_image_source=false
	do_params=false

	# done_{operation}=true if given operation has been done.
	#
	done_rebuildmake=false

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

	# Set the BUILDSEED to NetBSD-"N"
	#
	setmakeenv BUILDSEED "NetBSD-$(${HOST_SH} ${TOP}/sys/conf/osrelease.sh -m)"

	# Set MKARZERO to "yes"
	#
	setmakeenv MKARZERO "yes"

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

# nobomb_getmakevar --
# Given the name of a make variable in $1, print make's idea of the
# value of that variable, or return 1 if there's an error.
#
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

# nobomb_getmakevar --
# Given the name of a make variable in $1, print make's idea of the
# value of that variable, or bomb if there's an error.
#
bomb_getmakevar()
{
	[ -x "${make}" ] || bomb "bomb_getmakevar $1: ${make} is not executable"
	nobomb_getmakevar "$1" || bomb "bomb_getmakevar $1: ${make} failed"
}

# nobomb_getmakevar --
# Given the name of a make variable in $1, print make's idea of the
# value of that variable, or print a literal '$' followed by the
# variable name if ${make} is not executable.  This is intended for use in
# messages that need to be readable even if $make hasn't been built,
# such as when build.sh is run with the "-n" option.
#
getmakevar()
{
	if [ -x "${make}" ]; then
		bomb_getmakevar "$1"
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

# Given a variable name in $1, modify the variable in place as follows:
# For each space-separated word in the variable, call resolvepath.
resolvepaths()
{
	local var="$1"
	local val
	eval val=\"\${${var}}\"
	local newval=''
	local word
	for word in ${val}; do
		resolvepath word
		newval="${newval}${newval:+ }${word}"
	done
	eval ${var}=\"\${newval}\"
}

# Given a variable name in $1, modify the variable in place as follows:
# Convert possibly-relative path to absolute path by prepending
# ${TOP} if necessary.  Also delete trailing "/", if any.
resolvepath()
{
	local var="$1"
	local val
	eval val=\"\${${var}}\"
	case "${val}" in
	/)
		;;
	/*)
		val="${val%/}"
		;;
	*)
		val="${TOP}/${val%/}"
		;;
	esac
	eval ${var}=\"\${val}\"
}

usage()
{
	if [ -n "$*" ]; then
		echo ""
		echo "${progname}: $*"
	fi
	cat <<_usage_

Usage: ${progname} [-EnorUux] [-a arch] [-B buildid] [-C cdextras]
                [-D dest] [-j njob] [-M obj] [-m mach] [-N noisy]
                [-O obj] [-R release] [-S seed] [-T tools]
                [-V var=[value]] [-w wrapper] [-X x11src] [-Z var]
                operation [...]

 Build operations (all imply "obj" and "tools"):
    build               Run "make build".
    distribution        Run "make distribution" (includes DESTDIR/etc/ files).
    release             Run "make release" (includes kernels & distrib media).

 Other operations:
    help                Show this message and exit.
    makewrapper         Create ${toolprefix}make-\${MACHINE} wrapper and ${toolprefix}make.
                        Always performed.
    cleandir            Run "make cleandir".  [Default unless -u is used]
    obj                 Run "make obj".  [Default unless -o is used]
    tools               Build and install tools.
    install=idir        Run "make installworld" to \`idir' to install all sets
                        except \`etc'.  Useful after "distribution" or "release"
    kernel=conf         Build kernel with config file \`conf'
    releasekernel=conf  Install kernel built by kernel=conf to RELEASEDIR.
    modules             Build and install kernel modules.
    sets                Create binary sets in
                        RELEASEDIR/RELEASEMACHINEDIR/binary/sets.
                        DESTDIR should be populated beforehand.
    sourcesets          Create source sets in RELEASEDIR/source/sets.
    syspkgs             Create syspkgs in
                        RELEASEDIR/RELEASEMACHINEDIR/binary/syspkgs.
    iso-image           Create CD-ROM image in RELEASEDIR/iso.
    iso-image-source    Create CD-ROM image with source in RELEASEDIR/iso.
    params              Display various make(1) parameters.

 Options:
    -a arch     Set MACHINE_ARCH to arch.  [Default: deduced from MACHINE]
    -B buildId  Set BUILDID to buildId.
    -C cdextras Append cdextras to CDEXTRA variable for inclusion on CD-ROM.
    -D dest     Set DESTDIR to dest.  [Default: destdir.MACHINE]
    -E          Set "expert" mode; disables various safety checks.
                Should not be used without expert knowledge of the build system.
    -h          Print this help message.
    -j njob     Run up to njob jobs in parallel; see make(1) -j.
    -M obj      Set obj root directory to obj; sets MAKEOBJDIRPREFIX.
                Unsets MAKEOBJDIR.
    -m mach     Set MACHINE to mach; not required if NetBSD native.
    -N noisy    Set the noisyness (MAKEVERBOSE) level of the build:
                    0   Minimal output ("quiet")
                    1   Describe what is occurring
                    2   Describe what is occurring and echo the actual command
                    3   Ignore the effect of the "@" prefix in make commands
                    4   Trace shell commands using the shell's -x flag
                [Default: 2]
    -n          Show commands that would be executed, but do not execute them.
    -O obj      Set obj root directory to obj; sets a MAKEOBJDIR pattern.
                Unsets MAKEOBJDIRPREFIX.
    -o          Set MKOBJDIRS=no; do not create objdirs at start of build.
    -R release  Set RELEASEDIR to release.  [Default: releasedir]
    -r          Remove contents of TOOLDIR and DESTDIR before building.
    -S seed     Set BUILDSEED to seed.  [Default: NetBSD-majorversion]
    -T tools    Set TOOLDIR to tools.  If unset, and TOOLDIR is not set in
                the environment, ${toolprefix}make will be (re)built unconditionally.
    -U          Set MKUNPRIVED=yes; build without requiring root privileges,
                install from an UNPRIVED build with proper file permissions.
    -u          Set MKUPDATE=yes; do not run "make cleandir" first.
                Without this, everything is rebuilt, including the tools.
    -V v=[val]  Set variable \`v' to \`val'.
    -w wrapper  Create ${toolprefix}make script as wrapper.
                [Default: \${TOOLDIR}/bin/${toolprefix}make-\${MACHINE}]
    -X x11src   Set X11SRCDIR to x11src.  [Default: /usr/xsrc]
    -x          Set MKX11=yes; build X11 from X11SRCDIR
    -Z v        Unset ("zap") variable \`v'.

_usage_
	exit 1
}

parseoptions()
{
	opts='a:B:C:D:Ehj:M:m:N:nO:oR:rS:T:UuV:w:xX:Z:'
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

		-C)
			eval ${optargcmd}; resolvepaths OPTARG
			CDEXTRA="${CDEXTRA}${CDEXTRA:+ }${OPTARG}"
			;;

		-D)
			eval ${optargcmd}; resolvepath OPTARG
			setmakeenv DESTDIR "${OPTARG}"
			;;

		-E)
			do_expertmode=true
			;;

		-j)
			eval ${optargcmd}
			parallel="-j ${OPTARG}"
			;;

		-M)
			eval ${optargcmd}; resolvepath OPTARG
			case "${OPTARG}" in
			\$*)	usage "-M argument must not begin with '$'"
				;;
			*\$*)	# can use resolvepath, but can't set TOP_objdir
				resolvepath OPTARG
				;;
			*)	resolvepath OPTARG
				TOP_objdir="${OPTARG}${TOP}"
				;;
			esac
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
			0|1|2|3|4)
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
			eval ${optargcmd}
			case "${OPTARG}" in
			*\$*)	usage "-O argument must not contain '$'"
				;;
			*)	resolvepath OPTARG
				TOP_objdir="${OPTARG}"
				;;
			esac
			unsetmakeenv MAKEOBJDIRPREFIX
			setmakeenv MAKEOBJDIR "\${.CURDIR:C,^$TOP,$OPTARG,}"
			;;

		-o)
			MKOBJDIRS=no
			;;

		-R)
			eval ${optargcmd}; resolvepath OPTARG
			setmakeenv RELEASEDIR "${OPTARG}"
			;;

		-r)
			do_removedirs=true
			do_rebuildmake=true
			;;

		-S)
			eval ${optargcmd}
			setmakeenv BUILDSEED "${OPTARG}"
			;;

		-T)
			eval ${optargcmd}; resolvepath OPTARG
			TOOLDIR="${OPTARG}"
			export TOOLDIR
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
			eval ${optargcmd}; resolvepath OPTARG
			makewrapper="${OPTARG}"
			;;

		-X)
			eval ${optargcmd}; resolvepath OPTARG
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

		makewrapper|cleandir|obj|tools|build|distribution|release|sets|sourcesets|syspkgs|params)
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

		modules)
			op=modules
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

# print_tooldir_make --
# Try to find and print a path to an existing
# ${TOOLDIR}/bin/${toolprefix}make, for use by rebuildmake() before a
# new version of ${toolprefix}make has been built.
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
#   nobomb_getmakevar to find the correct value for TOOLDIR, and believe the
#   result only if it's a directory that already exists;
# * If a value of TOOLDIR was found above, and if
#   ${TOOLDIR}/bin/${toolprefix}make exists, print that value.
#
print_tooldir_make()
{
	local possible_TOP_OBJ
	local possible_TOOLDIR
	local possible_make
	local tooldir_make

	if [ -n "${TOOLDIR}" ]; then
		echo "${TOOLDIR}/bin/${toolprefix}make"
		return 0
	fi

	# Set host_ostype to something like "NetBSD-4.5.6-i386".  This
	# is intended to match the HOST_OSTYPE variable in <bsd.own.mk>.
	#
	local host_ostype="${uname_s}-$(
		echo "${uname_r}" | sed -e 's/([^)]*)//g' -e 's/ /_/g'
		)-$(
		echo "${uname_p}" | sed -e 's/([^)]*)//g' -e 's/ /_/g'
		)"

	# Look in a few potential locations for
	# ${possible_TOOLDIR}/bin/${toolprefix}make.
	# If we find it, then set possible_make.
	#
	# In the usual case (without interference from environment
	# variables or /etc/mk.conf), <bsd.own.mk> should set TOOLDIR to
	# "${_SRC_TOP_OBJ_}/tooldir.${host_ostype}".
	#
	# In practice it's difficult to figure out the correct value
	# for _SRC_TOP_OBJ_.  In the easiest case, when the -M or -O
	# options were passed to build.sh, then ${TOP_objdir} will be
	# the correct value.  We also try a few other possibilities, but
	# we do not replicate all the logic of <bsd.obj.mk>.
	#
	for possible_TOP_OBJ in \
		"${TOP_objdir}" \
		"${MAKEOBJDIRPREFIX:+${MAKEOBJDIRPREFIX}${TOP}}" \
		"${TOP}" \
		"${TOP}/obj" \
		"${TOP}/obj.${MACHINE}"
	do
		[ -n "${possible_TOP_OBJ}" ] || continue
		possible_TOOLDIR="${possible_TOP_OBJ}/tooldir.${host_ostype}"
		possible_make="${possible_TOOLDIR}/bin/${toolprefix}make"
		if [ -x "${possible_make}" ]; then
			break
		else
			unset possible_make
		fi
	done

	# If the above didn't work, search the PATH for a suitable
	# ${toolprefix}make, nbmake, bmake, or make.
	#
	: ${possible_make:=$(find_in_PATH ${toolprefix}make '')}
	: ${possible_make:=$(find_in_PATH nbmake '')}
	: ${possible_make:=$(find_in_PATH bmake '')}
	: ${possible_make:=$(find_in_PATH make '')}

	# At this point, we don't care whether possible_make is in the
	# correct TOOLDIR or not; we simply want it to be usable by
	# getmakevar to help us find the correct TOOLDIR.
	#
	# Use ${possible_make} with nobomb_getmakevar to try to find
	# the value of TOOLDIR.  Believe the result only if it's
	# a directory that already exists and contains bin/${toolprefix}make.
	#
	if [ -x "${possible_make}" ]; then
		possible_TOOLDIR="$(
			make="${possible_make}" nobomb_getmakevar TOOLDIR
			)"
		if [ $? = 0 ] && [ -n "${possible_TOOLDIR}" ] \
		    && [ -d "${possible_TOOLDIR}" ];
		then
			tooldir_make="${possible_TOOLDIR}/bin/${toolprefix}make"
			if [ -x "${tooldir_make}" ]; then
				echo "${tooldir_make}"
				return 0
			fi
		fi
	fi
	return 1
}

# rebuildmake --
# Rebuild nbmake in a temporary directory if necessary.  Sets $make
# to a path to the nbmake executable.  Sets done_rebuildmake=true
# if nbmake was rebuilt.
#
# There is a cyclic dependency between building nbmake and choosing
# TOOLDIR: TOOLDIR may be affected by settings in /etc/mk.conf, so we
# would like to use getmakevar to get the value of TOOLDIR; but we can't
# use getmakevar before we have an up to date version of nbmake; we
# might already have an up to date version of nbmake in TOOLDIR, but we
# don't yet know where TOOLDIR is.
#
# The default value of TOOLDIR also depends on the location of the top
# level object directory, so $(getmakevar TOOLDIR) invoked before or
# after making the top level object directory may produce different
# results.
#
# Strictly speaking, we should do the following:
#
#    1. build a new version of nbmake in a temporary directory;
#    2. use the temporary nbmake to create the top level obj directory;
#    3. use $(getmakevar TOOLDIR) with the temporary nbmake to
#       get the corect value of TOOLDIR;
#    4. move the temporary nbake to ${TOOLDIR}/bin/nbmake.
#
# However, people don't like building nbmake unnecessarily if their
# TOOLDIR has not changed since an earlier build.  We try to avoid
# rebuilding a temporary version of nbmake by taking some shortcuts to
# guess a value for TOOLDIR, looking for an existing version of nbmake
# in that TOOLDIR, and checking whether that nbmake is newer than the
# sources used to build it.
#
rebuildmake()
{
	make="$(print_tooldir_make)"
	if [ -n "${make}" ] && [ -x "${make}" ]; then
		for f in usr.bin/make/*.[ch] usr.bin/make/lst.lib/*.[ch]; do
			if [ "${f}" -nt "${make}" ]; then
				statusmsg "${make} outdated" \
					"(older than ${f}), needs building."
				do_rebuildmake=true
				break
			fi
		done
	else
		statusmsg "No \$TOOLDIR/bin/${toolprefix}make, needs building."
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
		done_rebuildmake=true
	fi
}

validatemakeparams()
{
	if [ "${runcmd}" = "echo" ]; then
		TOOLCHAIN_MISSING=no
		EXTERNAL_TOOLCHAIN=""
	else
		TOOLCHAIN_MISSING=$(bomb_getmakevar TOOLCHAIN_MISSING)
		EXTERNAL_TOOLCHAIN=$(bomb_getmakevar EXTERNAL_TOOLCHAIN)
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
		# Create the top-level object directory.
		#
		# "make obj NOSUBDIR=" can handle most cases, but it
		# can't handle the case where MAKEOBJDIRPREFIX is set
		# while the corresponding directory does not exist
		# (rules in <bsd.obj.mk> would abort the build).  We
		# therefore have to handle the MAKEOBJDIRPREFIX case
		# without invoking "make obj".  The MAKEOBJDIR case
		# could be handled either way, but we choose to handle
		# it similarly to MAKEOBJDIRPREFIX.
		#
		if [ -n "${TOP_obj}" ]; then
			# It must have been set by the "-M" or "-O"
			# command line options, so there's no need to
			# use getmakevar
			:
		elif [ -n "$MAKEOBJDIRPREFIX" ]; then
			TOP_obj="$(getmakevar MAKEOBJDIRPREFIX)${TOP}"
		elif [ -n "$MAKEOBJDIR" ]; then
			TOP_obj="$(getmakevar MAKEOBJDIR)"
		fi
		if [ -n "$TOP_obj" ]; then
			${runcmd} mkdir -p "${TOP_obj}" ||
			    bomb "Can't create top level object directory" \
					"${TOP_obj}"
		else
			${runcmd} "${make}" -m ${TOP}/share/mk obj NOSUBDIR= ||
			    bomb "Can't create top level object directory" \
					"using make obj"
		fi

		# make obj in tools to ensure that the objdir for "tools"
		# is available.
		#
		${runcmd} cd tools
		${runcmd} "${make}" -m ${TOP}/share/mk obj NOSUBDIR= ||
		    bomb "Failed to make obj in tools"
		${runcmd} cd "${TOP}"
	fi

	# Find TOOLDIR, DESTDIR, RELEASEDIR, and RELEASEMACHINEDIR.
	# This must be done after creating the top-level object directory.
	#
	TOOLDIR=$(getmakevar TOOLDIR)
	statusmsg "TOOLDIR path:     ${TOOLDIR}"
	DESTDIR=$(getmakevar DESTDIR)
	RELEASEDIR=$(getmakevar RELEASEDIR)
	RELEASEMACHINEDIR=$(getmakevar RELEASEMACHINEDIR)
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
		    [ "$id_u" -ne 0 ] && \
		    [ "${MKUNPRIVED}" = "no" ] ; then
			bomb "-U or -E must be set for build as an unprivileged user."
		fi
	fi
	if ${do_releasekernel} && [ -z "${RELEASEDIR}" ]; then
		bomb "Must set RELEASEDIR with \`releasekernel=...'"
	fi

	# Install as non-root is a bad idea.
	#
	if ${do_install} && [ "$id_u" -ne 0 ] ; then
		if ${do_expertmode}; then
			warning "Will install as an unprivileged user."
		else
			bomb "-E must be set for install as an unprivileged user."
		fi
	fi

	# If a previous build.sh run used -U (and therefore created a
	# METALOG file), then most subsequent build.sh runs must also
	# use -U.  If DESTDIR is about to be removed, then don't perform
	# this check.
	#
	case "${do_removedirs} ${removedirs} " in
	true*" ${DESTDIR} "*)
		# DESTDIR is about to be removed
		;;
	*)
		if ( ${do_build} || ${do_distribution} || ${do_release} || \
		    ${do_install} ) && \
		    [ -e "${DESTDIR}/METALOG" ] && \
		    [ "${MKUNPRIVED}" = "no" ] ; then
			if $do_expertmode; then
				warning "A previous build.sh run specified -U."
			else
				bomb "A previous build.sh run specified -U; you must specify it again now."
			fi
		fi
		;;
	esac
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
	if ${done_rebuildmake}; then
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
# Generated from:  \$NetBSD: build.sh,v 1.213 2009/09/27 22:02:41 apb Exp $
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

make_in_dir()
{
	dir="$1"
	op="$2"
	${runcmd} cd "${dir}" ||
	    bomb "Failed to cd to \"${dir}\""
	${runcmd} "${makewrapper}" ${parallel} ${op} ||
	    bomb "Failed to make ${op} in \"${dir}\""
	${runcmd} cd "${TOP}" ||
	    bomb "Failed to cd back to \"${TOP}\""
}

buildtools()
{
	if [ "${MKOBJDIRS}" != "no" ]; then
		${runcmd} "${makewrapper}" ${parallel} obj-tools ||
		    bomb "Failed to make obj-tools"
	fi
	if [ "${MKUPDATE}" = "no" ]; then
		make_in_dir tools cleandir
	fi
	make_in_dir tools dependall
	make_in_dir tools install
	statusmsg "Tools built to ${TOOLDIR}"
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
		make_in_dir "${KERNSRCDIR}/${KERNARCHDIR}/compile" obj
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
		make_in_dir "${kernelbuildpath}" cleandir
	fi
	[ -x "${TOOLDIR}/bin/${toolprefix}config" ] \
	|| bomb "${TOOLDIR}/bin/${toolprefix}config does not exist. You need to \"$0 tools\" first."
	${runcmd} "${TOOLDIR}/bin/${toolprefix}config" -b "${kernelbuildpath}" \
		-s "${TOP}/sys" "${kernelconfpath}" ||
	    bomb "${toolprefix}config failed for ${kernelconf}"
	make_in_dir "${kernelbuildpath}" depend
	make_in_dir "${kernelbuildpath}" all

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
	kernelreldir="${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/kernel"
	${runcmd} mkdir -p "${kernelreldir}"
	kernlist=$(awk '$1 == "config" { print $2 }' ${kernelconfpath})
	for kern in ${kernlist:-netbsd}; do
		builtkern="${kernelbuildpath}/${kern}"
		[ -f "${builtkern}" ] || continue
		releasekern="${kernelreldir}/${kern}-${kernelconfname}.gz"
		statusmsg "Kernel copy:      ${releasekern}"
		if [ "${runcmd}" = "echo" ]; then
			echo "gzip -c -9 < ${builtkern} > ${releasekern}"
		else
			gzip -c -9 < "${builtkern}" > "${releasekern}"
		fi
	done
}

buildmodules()
{
	if ! ${do_tools} && ! ${buildmoduleswarned:-false}; then
		# Building tools every time we build modules is clearly
		# unnecessary as well as a kernel.
		#
		statusmsg "Building modules without building new tools"
		buildmoduleswarned=true
	fi

	statusmsg "Building kernel modules for NetBSD/${MACHINE} ${DISTRIBVER}"
	if [ "${MKOBJDIRS}" != "no" ]; then
		make_in_dir sys/modules obj ||
		    bomb "Failed to make obj in sys/modules"
	fi
	if [ "${MKUPDATE}" = "no" ]; then
		make_in_dir sys/modules cleandir
	fi
	${runcmd} "${makewrapper}" ${parallel} do-sys-modules ||
	    bomb "Failed to make do-sys-modules"

	statusmsg "Successful build kernel modules for NetBSD/${MACHINE} ${DISTRIBVER}"
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
			setdir=${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/sets
			statusmsg "Built sets to ${setdir}"
			;;

		cleandir|obj|build|distribution|release|sourcesets|syspkgs|params)
			${runcmd} "${makewrapper}" ${parallel} ${op} ||
			    bomb "Failed to make ${op}"
			statusmsg "Successful make ${op}"
			;;

		iso-image|iso-image-source)
			${runcmd} "${makewrapper}" ${parallel} \
			    CDEXTRA="$CDEXTRA" ${op} ||
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

		modules)
			buildmodules
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
