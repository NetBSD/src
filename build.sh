#! /bin/sh
#  $NetBSD: build.sh,v 1.10 2001/10/30 22:33:00 tv Exp $
#
# Top level build wrapper, for a system containing no tools.
#
# This script should run on any POSIX-compliant shell.  For systems
# with a strange /bin/sh, "ksh" may be an ample alternative.
#

bomb () {
	echo ""
	echo "ERROR: $@"
	echo "*** BUILD ABORTED ***"
	exit 1
}

getarch () {
	# Translate a MACHINE into a default MACHINE_ARCH.
	case $MACHINE in
		arm26|dnard|evbarm|hpcarm|netwinder)
			MACHINE_ARCH=arm;;

		acorn32|arm32|cats)
			MACHINE_ARCH=arm32;;

		sun2)
			MACHINE_ARCH=m68000;;

		amiga|atari|cesfic|hp300|sun3|*68k)
			MACHINE_ARCH=m68k;;

		mipsco|newsmips|sgimips)
			MACHINE_ARCH=mipseb;;

		algor|arc|cobalt|hpcmips|playstation2|pmax)
			MACHINE_ARCH=mipsel;;

		pc532)
			MACHINE_ARCH=ns32k;;

		bebox|prep|sandpoint|walnut|*ppc)
			MACHINE_ARCH=powerpc;;

		mmeye)
			MACHINE_ARCH=sh3eb;;

		dreamcast|evbsh3|hpcsh)
			MACHINE_ARCH=sh3el;;

		alpha|i386|sparc|sparc64|vax|x86_64)
			MACHINE_ARCH=$MACHINE;;

		*)	bomb "unknown target MACHINE: $MACHINE";;
	esac
}

# Emulate "mkdir -p" for systems that have an Old "mkdir".
mkdirp () {
	IFS=/; set -- $@; unset IFS
	_d=
	if [ "$1" = "" ]; then _d=/; shift; fi

	for _f in "$@"; do
		if [ "$_f" != "" ]; then
			[ -d "$_d$_f" ] || $runcmd mkdir "$_d$_f" || return 1
			_d="$_d$_f/"
		fi
	done
}

resolvepath () {
	case $OPTARG in
	/*)	;;
	*)	OPTARG="$cwd/$OPTARG";;
	esac
}

usage () {
	echo "Usage:"
	echo "$0 [-r] [-a arch] [-j njob] [-m mach] [-D dest] [-R release] [-T tools]"
	echo "    -m: set target MACHINE to mach (REQUIRED)"
	echo "    -D: set DESTDIR to dest (REQUIRED unless -b is specified)"
	echo "    -T: set TOOLDIR to tools (REQUIRED)"
	echo ""
	echo "    -a: set target MACHINE_ARCH to arch (otherwise deduced from MACHINE)"
	echo "    -b: do not build the system; just build nbmake if required."
	echo "    -j: set NBUILDJOBS to njob"
	echo "    -n: show the commands that would be executed, but do not execute them"
	echo "    -r: remove TOOLDIR and DESTDIR before the build"
	echo "    -R: build a release (set RELEASEDIR) to release"
	exit 1
}

# Set defaults.
cwd=`pwd`
do_buildsystem=true
do_rebuildmake=false
do_removedirs=false
opt_a=no
opts='a:bhj:m:nrD:R:T:'
runcmd=''

if type getopts >/dev/null 2>&1; then
	# Use POSIX getopts.
	getoptcmd='getopts $opts opt && opt=-$opt'
	optargcmd=':'
else
	type getopt >/dev/null 2>&1 || bomb "/bin/sh shell is too old; try ksh"

	# Use old-style getopt(1) (doesn't handle whitespace in args).
	args="`getopt $opts $*`"
	[ $? = 0 ] || usage
	set -- $args

	getoptcmd='[ $# -gt 0 ] && opt="$1" && shift'
	optargcmd='OPTARG="$1"; shift'
fi

# Parse command line options.
while eval $getoptcmd; do case $opt in
	-a)	eval $optargcmd
		MACHINE_ARCH=$OPTARG; opt_a=yes;;

	-b)	do_buildsystem=false;;

	-j)	eval $optargcmd
		buildjobflag="NBUILDJOBS=$OPTARG";;

	# -m overrides MACHINE_ARCH unless "-a" is specified
	-m)	eval $optargcmd
		MACHINE=$OPTARG; [ "$opt_a" != "yes" ] && getarch;;

	-n)	runcmd=echo;;

	-r)	do_removedirs=true; do_rebuildmake=true;;

	-D)	eval $optargcmd; resolvepath
		DESTDIR="$OPTARG";;

	-R)	eval $optargcmd; resolvepath
		releasedirflag="RELEASEDIR=$OPTARG"; buildtarget=release;;

	-T)	eval $optargcmd; resolvepath
		TOOLDIR="$OPTARG";;

	--)		break;;
	-'?'|-h)	usage;;
esac; done

# Check required environment; DESTDIR only needed if building.
checkvars='MACHINE TOOLDIR'
$do_buildsystem && checkvars="$checkvars DESTDIR"

for var in $checkvars; do
	if ! eval '[ -n "$'$var'" ]'; then
		echo "$var must be set in the environment before running build.sh."
		echo ""; usage
	fi
done

# Set up environment.
[ -n "$MACHINE_ARCH" ] || getarch
[ -d usr.bin/make ] || bomb "build.sh must be run from the top source level"

# Remove the target directories.
if $do_removedirs; then
	echo "Removing DESTDIR and TOOLDIR...."
	$runcmd rm -rf $DESTDIR $TOOLDIR
fi

mkdirp $TOOLDIR/bin || bomb "mkdir of $TOOLDIR/bin failed"

# Test make source file timestamps against installed nbmake binary.
if [ -x $TOOLDIR/bin/nbmake ]; then
	for f in usr.bin/make/*.[ch] usr.bin/make/lst.lib/*.[ch]; do
		if [ $f -nt $TOOLDIR/bin/nbmake ]; then
			do_rebuildmake=true; break
		fi
	done
else
	do_rebuildmake=true
fi

# Build $TOOLDIR/bin/nbmake.
if $do_rebuildmake; then
	echo "Building nbmake...."

	# Go to a temporary directory in case building .o's happens.
	tmpdir=${TMPDIR-/tmp}/nbbuild$$

	$runcmd mkdir $tmpdir || bomb "cannot mkdir: $tmpdir"
	trap "rm -r -f $tmpdir" 0
	trap "exit 1" 1 2 3 15
	$runcmd cd $tmpdir

	${runcmd-eval} "${HOST_CC-cc} ${HOST_CFLAGS} -DMAKE_BOOTSTRAP \
		-o $TOOLDIR/bin/nbmake -I$cwd/usr.bin/make \
		$cwd/usr.bin/make/*.c $cwd/usr.bin/make/lst.lib/*.c" \
		|| bomb "build of nbmake failed"

	# Clean up.
	$runcmd cd $cwd
	$runcmd rm -r -f $tmpdir
	trap 0 1 2 3 15

	# Some compilers are just *that* braindead.
	${runcmd-eval} "rm -f $cwd/usr.bin/make/*.o \
		$cwd/usr.bin/make/lst.lib/*.o"
fi

# Build a nbmake wrapper script, usable by hand as well as by build.sh.
makeprog=$TOOLDIR/bin/nbmake-$MACHINE

if $do_rebuildmake || [ ! -f $makeprog ] || [ $makeprog -ot build.sh ]; then
	rm -f $makeprog
	if [ "$runcmd" = "echo" ]; then
		mkscriptcmd='echo "cat >$makeprog <<EOF" && cat'
	else
		mkscriptcmd="cat >$makeprog"
	fi
	eval $mkscriptcmd <<EOF
#! /bin/sh
# Set proper variables to allow easy "make" building of a NetBSD subtree.
# Generated from:  \$NetBSD: build.sh,v 1.10 2001/10/30 22:33:00 tv Exp $
#
exec $TOOLDIR/bin/nbmake MACHINE=$MACHINE MACHINE_ARCH=$MACHINE_ARCH \
USETOOLS=yes USE_NEW_TOOLCHAIN=yes TOOLDIR="$TOOLDIR" \${1+\$@}
EOF
	[ "$runcmd" = "echo" ] && echo EOF
	$runcmd chmod +x $makeprog
fi

if $do_buildsystem; then
	${runcmd-exec} $makeprog -m `pwd`/share/mk ${buildtarget-build} \
		MKTOOLS=yes DESTDIR="$DESTDIR" TOOLDIR="$TOOLDIR" \
		$buildjobflag $releasedirflag
fi
