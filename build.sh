#! /usr/bin/env sh
#	$NetBSD: build.sh,v 1.87 2003/01/26 06:19:12 lukem Exp $
#
# Copyright (c) 2001-2003 The NetBSD Foundation, Inc.
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
# This script should run on any POSIX-compliant shell.  For systems
# with a strange /bin/sh, "ksh" or "bash" may be an ample alternative.
#
# Note, however, that due to the way the interpreter is invoked above,
# if a POSIX-compliant shell is the first in the PATH, you won't have
# to take any further action.
#

progname=${0##*/}
toppid=$$
trap "exit 1" 1 2 3 15

bomb()
{
	cat >&2 <<ERRORMESSAGE

ERROR: $@
*** BUILD ABORTED ***
ERRORMESSAGE
	kill $toppid		# in case we were invoked from a subshell
	exit 1
}


initdefaults()
{
	cd "$(dirname $0)"
	[ -d usr.bin/make ] ||
	    bomb "build.sh must be run from the top source level"
	[ -f share/mk/bsd.own.mk ] ||
	    bomb "src/share/mk is missing; please re-fetch the source tree"

	uname_s=$(uname -s 2>/dev/null)
	uname_m=$(uname -m 2>/dev/null)

	# If $PWD is a valid name of the current directory, POSIX mandates
	# that pwd return it by default which causes problems in the
	# presence of symlinks.  Unsetting PWD is simpler than changing
	# every occurrence of pwd to use -P.
	#
	# XXX Except that doesn't work on Solaris.
	unset PWD
	if [ "${uname_s}" = "SunOS" ]; then
		TOP=$(pwd -P)
	else
		TOP=$(pwd)
	fi

	# Set defaults.
	toolprefix=nb
	MAKEFLAGS=
	makeenv=
	makewrapper=
	runcmd=
	operations=
	removedirs=
	do_expertmode=false
	do_rebuildmake=false
	do_removedirs=false

	# do_{operation}=true if given operation is requested.
	#
	do_tools=false
	do_obj=false
	do_build=false
	do_distribution=false
	do_release=false
	do_kernel=false
	do_install=false
	do_sets=false
}

getarch()
{
	# Translate a MACHINE into a default MACHINE_ARCH.
	#
	case $MACHINE in

	acorn26|acorn32|cats|netwinder|shark|*arm)
		MACHINE_ARCH=arm
		;;

	sun2)
		MACHINE_ARCH=m68000
		;;

	amiga|atari|cesfic|hp300|sun3|*68k)
		MACHINE_ARCH=m68k
		;;

	mipsco|newsmips|sbmips|sgimips)
		MACHINE_ARCH=mipseb
		;;

	algor|arc|cobalt|evbmips|hpcmips|playstation2|pmax)
		MACHINE_ARCH=mipsel
		;;

	pc532)
		MACHINE_ARCH=ns32k
		;;

	bebox|prep|sandpoint|*ppc)
		MACHINE_ARCH=powerpc
		;;

	evbsh3|mmeye)
		MACHINE_ARCH=sh3eb
		;;

	dreamcast|hpcsh)
		MACHINE_ARCH=sh3el
		;;

	hp700)
		MACHINE_ARCH=hppa
		;;

	evbsh5)
		MACHINE_ARCH=sh5el
		;;

	alpha|i386|sparc|sparc64|vax|x86_64)
		MACHINE_ARCH=$MACHINE
		;;

	*)
		bomb "unknown target MACHINE: $MACHINE"
		;;

	esac
}

validatearch()
{
	# Ensure that the MACHINE_ARCH exists (and is supported by build.sh).
	#
	case $MACHINE_ARCH in

	alpha|arm|armeb|hppa|i386|m68000|m68k|mipse[bl]|ns32k|powerpc|sh[35]e[bl]|sparc|sparc64|vax|x86_64)
		;;

	*)
		bomb "unknown target MACHINE_ARCH: $MACHINE_ARCH"
		;;

	esac
}

getmakevar()
{
	[ -x $make ] || bomb "getmakevar $1: $make is not executable"
	$make -m ${TOP}/share/mk -s -f- _x_ <<EOF
_x_:
	echo \${$1}
.include <bsd.prog.mk>
.include <bsd.kernobj.mk>
EOF
}

safe_getmakevar()
{
	# getmakevar() doesn't work properly if $make hasn't yet been built,
	# which can happen when running with the "-n" option.
	# safe_getmakevar() deals with this by emitting a literal '$'
	# followed by the variable name, instead of trying to find the
	# variable's value.
	#

	if [ -x $make ]; then
		getmakevar "$1"
	else
		echo "\$$1"
	fi
}

resolvepath()
{
	case $OPTARG in
	/*)
		;;
	*)
		OPTARG="$TOP/$OPTARG"
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

Usage: ${progname} [-EnorUu] [-a arch] [-B buildid] [-D dest] [-j njob] [-M obj]
		[-m mach] [-O obj] [-R release] [-T tools] [-V var=[value]]
		[-w wrapper]   operation [...]

 Build operations (all imply "obj" and "tools"):
    build		Run "make build"
    distribution	Run "make distribution" (includes etc/ files)
    release		Run "make release" (includes kernels & distrib media)

 Other operations:
    help		Show this message (and exit)
    makewrapper		Create ${toolprefix}make-${MACHINE} wrapper and ${toolprefix}make.  (Always done)
    obj			Run "make obj" (default unless -o is used)
    tools 		Build and install tools
    kernel=conf		Build kernel with config file \`conf'
    install=idir	Run "make installworld" to \`idir'
			(useful after 'distribution' or 'release')

 Options:
    -a arch	Set MACHINE_ARCH to arch (otherwise deduced from MACHINE)
    -B buildId	Set BUILDID to buildId
    -D dest	Set DESTDIR to dest
    -E		Set "expert" mode; disables some DESTDIR checks
    -j njob	Run up to njob jobs in parallel; see make(1)
    -M obj	Set obj root directory to obj (sets MAKEOBJDIRPREFIX)
    -m mach	Set MACHINE to mach (not required if NetBSD native)
    -n		Show commands that would be executed, but do not execute them
    -O obj	Set obj root directory to obj (sets a MAKEOBJDIR pattern)
    -o		Set MKOBJDIRS=no (do not create objdirs at start of build)
    -R release	Set RELEASEDIR to release
    -r		Remove contents of TOOLDIR and DESTDIR before building
    -T tools	Set TOOLDIR to tools.  If unset, and TOOLDIR is not set in
		the environment, ${toolprefix}make will be (re)built unconditionally
    -U		Set UNPRIVED (build without requiring root privileges)
    -u		Set UPDATE (do not run "make clean" first)
    -V v=[val]	Set variable \`v' to \`val'
    -w wrapper	Create ${toolprefix}make script as wrapper
		(Default: \${TOOLDIR}/bin/${toolprefix}make-\${MACHINE})

_usage_
	exit 1
}

parseoptions()
{
	opts='a:B:bD:dEhi:j:k:M:m:nO:oR:rT:tUuV:w:'
	opt_a=no

	if type getopts >/dev/null 2>&1; then
		# Use POSIX getopts.
		getoptcmd='getopts $opts opt && opt=-$opt'
		optargcmd=':'
		optremcmd='shift $(($OPTIND -1))'
	else
		type getopt >/dev/null 2>&1 ||
		    bomb "/bin/sh shell is too old; try ksh or bash"

		# Use old-style getopt(1) (doesn't handle whitespace in args).
		args="$(getopt $opts $*)"
		[ $? = 0 ] || usage
		set -- $args

		getoptcmd='[ $# -gt 0 ] && opt="$1" && shift'
		optargcmd='OPTARG="$1"; shift'
		optremcmd=':'
	fi

	# Parse command line options.
	#
	while eval $getoptcmd; do
		case $opt in

		-a)
			eval $optargcmd
			MACHINE_ARCH=$OPTARG
			opt_a=yes
			;;

		-B)
			eval $optargcmd
			BUILDID=$OPTARG
			;;

		-b)
			usage "'-b' has been replaced by 'makewrapper'"
			;;

		-D)
			eval $optargcmd; resolvepath
			DESTDIR="$OPTARG"
			export DESTDIR
			makeenv="$makeenv DESTDIR"
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
			eval $optargcmd
			parallel="-j $OPTARG"
			;;

		-k)
			usage "'-k conf' has been replaced by 'kernel=conf'"
			;;

		-M)
			eval $optargcmd; resolvepath
			MAKEOBJDIRPREFIX="$OPTARG"
			export MAKEOBJDIRPREFIX
			makeobjdir=$OPTARG
			makeenv="$makeenv MAKEOBJDIRPREFIX"
			;;

			# -m overrides MACHINE_ARCH unless "-a" is specified
		-m)
			eval $optargcmd
			MACHINE=$OPTARG
			[ "$opt_a" != "yes" ] && getarch
			;;

		-n)
			runcmd=echo
			;;

		-O)
			eval $optargcmd; resolvepath
			MAKEOBJDIR="\${.CURDIR:C,^$TOP,$OPTARG,}"
			export MAKEOBJDIR
			makeobjdir=$OPTARG
			makeenv="$makeenv MAKEOBJDIR"
			;;

		-o)
			MKOBJDIRS=no
			;;

		-R)
			eval $optargcmd; resolvepath
			RELEASEDIR=$OPTARG
			export RELEASEDIR
			makeenv="$makeenv RELEASEDIR"
			;;

		-r)
			do_removedirs=true
			do_rebuildmake=true
			;;

		-T)
			eval $optargcmd; resolvepath
			TOOLDIR="$OPTARG"
			export TOOLDIR
			;;

		-t)
			usage "'-t' has been replaced by 'tools'"
			;;

		-U)
			UNPRIVED=yes
			export UNPRIVED
			makeenv="$makeenv UNPRIVED"
			;;

		-u)
			UPDATE=yes
			export UPDATE
			makeenv="$makeenv UPDATE"
			;;

		-V)
			eval $optargcmd
			case "${OPTARG}" in
		    # XXX: consider restricting which variables can be changed?
			[a-zA-Z_][a-zA-Z_0-9]*=*)
				var=${OPTARG%%=*}
				value=${OPTARG#*=}
				eval "${var}=\"${value}\"; export ${var}"
				makeenv="$makeenv ${var}"
				;;
			*)
				usage "-V argument must be of the form 'var=[value]'"
				;;
			esac
			;;

		-w)
			eval $optargcmd; resolvepath
			makewrapper="$OPTARG"
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
	eval $optremcmd
	while [ $# -gt 0 ]; do
		op=$1; shift
		operations="$operations $op"

		case "$op" in

		help)
			usage
			;;

		makewrapper|obj|tools|build|distribution|release|sets)
			;;

		kernel=*)
			arg=${op#*=}
			op=${op%%=*}
			if [ -z "${arg}" ]; then
				bomb "Must supply a kernel name with \`kernel=...'"
			fi
			;;

		install=*)
			arg=${op#*=}
			op=${op%%=*}
			if [ -z "${arg}" ]; then
				bomb "Must supply a directory with \`install=...'"
			fi
			;;

		*)
			usage "Unknown operation \`${op}'"
			;;

		esac
		eval do_$op=true
	done
	if [ -z "${operations}" ]; then
		usage "Missing operation to perform."
	fi

	# Set up MACHINE*.  On a NetBSD host, these are allowed to be unset.
	#
	if [ -z "$MACHINE" ]; then
		if [ "${uname_s}" != "NetBSD" ]; then
			bomb "MACHINE must be set, or -m must be used, for cross builds."
		fi
		MACHINE=${uname_m}
	fi
	[ -n "$MACHINE_ARCH" ] || getarch
	validatearch

	# Set up default make(1) environment.
	#
	makeenv="$makeenv TOOLDIR MACHINE MACHINE_ARCH MAKEFLAGS"
	if [ ! -z "$BUILDID" ]; then
		makeenv="$makeenv BUILDID"
	fi
	MAKEFLAGS="-m $TOP/share/mk $MAKEFLAGS MKOBJDIRS=${MKOBJDIRS-yes}"
	export MAKEFLAGS MACHINE MACHINE_ARCH
}

rebuildmake()
{
	# Test make source file timestamps against installed ${toolprefix}make
	# binary, if TOOLDIR is pre-set.
	#
	# Note that we do NOT try to grovel "mk.conf" here to find out if
	# TOOLDIR is set there, because it can contain make variable
	# expansions and other stuff only parseable *after* we have a working
	# ${toolprefix}make.  So this logic can only work if the user has
	# pre-set TOOLDIR in the environment or used the -T option to build.sh.
	#
	make="${TOOLDIR-nonexistent}/bin/${toolprefix}make"
	if [ -x $make ]; then
		for f in usr.bin/make/*.[ch] usr.bin/make/lst.lib/*.[ch]; do
			if [ $f -nt $make ]; then
				do_rebuildmake=true
				break
			fi
		done
	else
		do_rebuildmake=true
	fi

	# Build bootstrap ${toolprefix}make if needed.
	if $do_rebuildmake; then
		$runcmd echo "===> Bootstrapping ${toolprefix}make"
		tmpdir="${TMPDIR-/tmp}/nbbuild$$"

		$runcmd mkdir "$tmpdir" || bomb "cannot mkdir: $tmpdir"
		trap "cd /; rm -r -f \"$tmpdir\"" 0
		$runcmd cd "$tmpdir"

		$runcmd env CC="${HOST_CC-cc}" CPPFLAGS="${HOST_CPPFLAGS}" \
			CFLAGS="${HOST_CFLAGS--O}" LDFLAGS="${HOST_LDFLAGS}" \
			"$TOP/tools/make/configure" ||
		    bomb "configure of ${toolprefix}make failed"
		$runcmd sh buildmake.sh ||
		    bomb "build of ${toolprefix}make failed"

		make="$tmpdir/${toolprefix}make"
		$runcmd cd "$TOP"
		$runcmd rm -f usr.bin/make/*.o usr.bin/make/lst.lib/*.o
	fi
}

validatemakeparams()
{
	if [ "$runcmd" = "echo" ]; then
		TOOLCHAIN_MISSING=no
		EXTERNAL_TOOLCHAIN=""
	else
		TOOLCHAIN_MISSING=$(getmakevar TOOLCHAIN_MISSING)
		EXTERNAL_TOOLCHAIN=$(getmakevar EXTERNAL_TOOLCHAIN)
	fi
	if [ "${TOOLCHAIN_MISSING}" = "yes" ] && \
	   [ -z "${EXTERNAL_TOOLCHAIN}" ]; then
		$runcmd echo "ERROR: build.sh (in-tree cross-toolchain) is not yet available for"
		$runcmd echo "	MACHINE:      ${MACHINE}"
		$runcmd echo "	MACHINE_ARCH: ${MACHINE_ARCH}"
		$runcmd echo ""
		$runcmd echo "All builds for this platform should be done via a traditional make"
		$runcmd echo "If you wish to use an external cross-toolchain, set"
		$runcmd echo "	EXTERNAL_TOOLCHAIN=<path to toolchain root>"
		$runcmd echo "in either the environment or mk.conf and rerun"
		$runcmd echo "	$progname $*"
		exit 1
	fi

	# If TOOLDIR isn't already set, make objdirs in "tools" in case the
	# default setting from <bsd.own.mk> is used.
	#
	if [ -z "$TOOLDIR" ] && [ "$MKOBJDIRS" != "no" ]; then
		$runcmd cd tools
		$runcmd $make -m ${TOP}/share/mk obj NOSUBDIR= ||
		    bomb "make obj failed in tools"
		$runcmd cd "$TOP"
	fi

	# If setting -M or -O to root an obj dir make sure the base directory
	# is made before continuing as bsd.own.mk will need this to pick up
	# _SRC_TOP_OBJ_
	#
	if [ "$MKOBJDIRS" != "no" ] && [ ! -z "$makeobjdir" ]; then
		$runcmd mkdir -p "$makeobjdir"
	fi

	# Find DESTDIR and TOOLDIR.
	#
	DESTDIR=$(safe_getmakevar DESTDIR)
	$runcmd echo "===> DESTDIR path: $DESTDIR"
	TOOLDIR=$(safe_getmakevar TOOLDIR)
	$runcmd echo "===> TOOLDIR path: $TOOLDIR"
	export DESTDIR TOOLDIR

	# Check validity of TOOLDIR and DESTDIR.
	#
	if [ -z "$TOOLDIR" ] || [ "$TOOLDIR" = "/" ]; then
		bomb "TOOLDIR '$TOOLDIR' invalid"
	fi
	removedirs="$TOOLDIR"

	if [ -z "$DESTDIR" ] || [ "$DESTDIR" = "/" ]; then
		if $do_build || $do_distribution || $do_release; then
			if ! $do_build || \
			   [ "${uname_s}" != "NetBSD" ] || \
			   [ "${uname_m}" != "$MACHINE" ]; then
				bomb "DESTDIR must != / for cross builds, or ${progname} 'distribution' or 'release'."
			fi
			if ! $do_expertmode; then
				bomb "DESTDIR must != / for non -E (expert) builds"
			fi
			$runcmd echo "===> WARNING: Building to /, in expert mode."
			$runcmd echo "===>          This may cause your system to break!  Reasons include:"
			$runcmd echo "===>             - your kernel is not up to date"
			$runcmd echo "===>             - the libraries or toolchain have changed"
			$runcmd echo "===>          YOU HAVE BEEN WARNED!"
		fi
	else
		removedirs="$removedirs $DESTDIR"
	fi
	if $do_build || $do_distribution || $do_release; then
		if ! $do_expertmode && \
		    [ $(id -u 2>/dev/null) -ne 0 ] &&\
		    [ -z "$UNPRIVED" ] ; then
			bomb "-U or -E must be set for build as an unprivileged user."
		fi
        fi
}


createmakewrapper()
{
	# Remove the target directories.
	#
	if $do_removedirs; then
		for f in $removedirs; do
			$runcmd echo "===> Removing $f"
			$runcmd rm -r -f $f
		done
	fi

	# Recreate $TOOLDIR.
	#
	$runcmd mkdir -p "$TOOLDIR/bin" || bomb "mkdir of '$TOOLDIR/bin' failed"

	# Install ${toolprefix}make if it was built.
	#
	if $do_rebuildmake; then
		$runcmd rm -f "$TOOLDIR/bin/${toolprefix}make"
		$runcmd cp $make "$TOOLDIR/bin/${toolprefix}make" ||
		    bomb "failed to install \$TOOLDIR/bin/${toolprefix}make"
		make="$TOOLDIR/bin/${toolprefix}make"
		$runcmd echo "===> Created ${make}"
		$runcmd rm -r -f "$tmpdir"
		trap 0
	fi

	# Build a ${toolprefix}make wrapper script, usable by hand as
	# well as by build.sh.
	#
	if [ -z "$makewrapper" ]; then
		makewrapper="$TOOLDIR/bin/${toolprefix}make-$MACHINE"
		if [ ! -z "$BUILDID" ]; then
			makewrapper="$makewrapper-$BUILDID"
		fi
	fi

	$runcmd rm -f "$makewrapper"
	if [ "$runcmd" = "echo" ]; then
		echo 'cat <<EOF >'$makewrapper
		makewrapout=
	else
		makewrapout=">>\$makewrapper"
	fi

	eval cat <<EOF $makewrapout
#! /bin/sh
# Set proper variables to allow easy "make" building of a NetBSD subtree.
# Generated from:  \$NetBSD: build.sh,v 1.87 2003/01/26 06:19:12 lukem Exp $
#

EOF
	for f in $makeenv; do
		eval echo "$f=\'\$$(echo $f)\'\;\ export\ $f" $makewrapout
	done
	eval echo "USETOOLS=yes\; export USETOOLS" $makewrapout

	eval cat <<EOF $makewrapout

exec "\$TOOLDIR/bin/${toolprefix}make" \${1+"\$@"}
EOF
	[ "$runcmd" = "echo" ] && echo EOF
	$runcmd chmod +x "$makewrapper"
	$runcmd echo "===> Updated ${makewrapper}"
}

buildtools()
{
	if [ "$MKOBJDIRS" != "no" ]; then
		$runcmd "$makewrapper" $parallel obj-tools ||
		    bomb "failed to make obj-tools"
	fi
	$runcmd cd tools
	if [ -z "$UPDATE" ]; then
		cleandir=cleandir
	else
		cleandir=
	fi
	$runcmd "$makewrapper" ${cleandir} dependall install ||
	    bomb "failed to make tools"
}

buildkernel()
{
	kernconf="$1"
	if ! $do_tools; then
		# Building tools every time we build a kernel is clearly
		# unnecessary.  We could try to figure out whether rebuilding
		# the tools is necessary this time, but it doesn't seem worth
		# the trouble.  Instead, we say it's the user's responsibility
		# to rebuild the tools if necessary.
		#
		$runcmd echo "===> Building kernel without building new tools"
	fi
	$runcmd echo "===> Building kernel ${kernconf}"
	if [ "$MKOBJDIRS" != "no" ] && [ ! -z "$makeobjdir" ]; then
		# The correct value of KERNOBJDIR might
		# depend on a prior "make obj" in
		# ${KERNSRCDIR}/${KERNARCHDIR}/compile.
		#
		KERNSRCDIR="$(safe_getmakevar KERNSRCDIR)"
		KERNARCHDIR="$(safe_getmakevar KERNARCHDIR)"
		$runcmd cd "${KERNSRCDIR}/${KERNARCHDIR}/compile"
		$runcmd "$makewrapper" obj ||
		    bomb "failed to make obj in ${KERNSRCDIR}/${KERNARCHDIR}/compile"
		$runcmd cd "$TOP"
	fi
	KERNCONFDIR="$(safe_getmakevar KERNCONFDIR)"
	KERNOBJDIR="$(safe_getmakevar KERNOBJDIR)"
	case "${kernconf}" in
	*/*)
		kernconfpath="${kernconf}"
		kernconfbase="$(basename "${kernconf}")"
		;;
	*)
		kernconfpath="${KERNCONFDIR}/${kernconf}"
		kernconfbase="${kernconf}"
		;;
	esac
	kernbuilddir="${KERNOBJDIR}/${kernconfbase}"
	$runcmd echo "===> Kernel build directory: ${kernbuilddir}"
	$runcmd mkdir -p "${kernbuilddir}" ||
	    bomb "cannot mkdir: ${kernbuilddir}"
	if [ -z "$UPDATE" ]; then
		$runcmd cd "${kernbuilddir}"
		$runcmd "$makewrapper" cleandir ||
		    bomb "make cleandir failed in ${kernbuilddir}"
		$runcmd cd "$TOP"
	fi
	$runcmd "${TOOLDIR}/bin/${toolprefix}config" -b "${kernbuilddir}" \
		-s "${TOP}/sys" "${kernconfpath}" ||
	    bomb "${toolprefix}config failed for ${kernconf}"
	$runcmd cd "${kernbuilddir}"
	$runcmd "$makewrapper" depend ||
	    bomb "make depend failed in ${kernbuilddir}"
	$runcmd "$makewrapper" $parallel all ||
	    bomb "make all failed in ${kernbuilddir}"
	$runcmd echo "===> New kernel should be in:"
	$runcmd echo "	${kernbuilddir}"
}

installworld()
{
	dir="$1"
	${runcmd} "$makewrapper" INSTALLWORLDDIR="${dir}" installworld ||
	    bomb "failed to make installworld to ${dir}"
}


main()
{
	initdefaults
	parseoptions "$@"
	rebuildmake
	validatemakeparams
	createmakewrapper

	# Perform the operations.
	#
	for op in $operations; do
		case "$op" in

		makewrapper)
			# no-op
			;;

		tools)
			buildtools
			;;

		obj|build|distribution|release|sets)
			${runcmd} "$makewrapper" $parallel $op ||
			    bomb "failed to make $op"
			;;

		kernel=*)
			arg=${op#*=}
			buildkernel "${arg}"
			;;

		install=*)
			arg=${op#*=}
			if [ "${arg}" = "/" ] && \
			    (	[ "${uname_s}" != "NetBSD" ] || \
				[ "${uname_m}" != "$MACHINE" ] ); then
				bomb "'${op}' must != / for cross builds."
			fi
			installworld "${arg}"
			;;

		*)
			bomb "Unknown operation \`${op}'"
			;;

		esac
	done
}

main "$@"
