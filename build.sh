#! /bin/sh
#  $NetBSD: build.sh,v 1.6 2001/10/24 14:55:09 bjh21 Exp $
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
		arm26|dnard|evbarm|hpcarm)
			MACHINE_ARCH=arm;;

		acorn32|arm32|cats|netwinder)
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
			[ -d "$_d$_f" ] || mkdir "$_d$_f" || return 1
			_d="$_d$_f/"
		fi
	done
}

usage () {
	echo "Usage:"
	echo "$0 [-r] [-a arch] [-j njob] [-m mach] [-D dest] [-R release] [-T tools]"
	echo "    -m: set target MACHINE to mach (REQUIRED, or set in environment)"
	echo "    -D: set DESTDIR to dest (REQUIRED, or set in environment)"
	echo "    -T: set TOOLDIR to tools (REQUIRED, or set in environment)"
	echo ""
	echo "    -a: set target MACHINE_ARCH to arch (otherwise deduced from MACHINE)"
	echo "    -j: set NBUILDJOBS to njob"
	echo "    -r: remove TOOLDIR and DESTDIR before the build"
	echo "    -R: build a release (set RELEASEDIR) to release"
	exit 1
}

opts='a:hj:m:rD:R:T:'

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
	
opt_a=no
while eval $getoptcmd; do case $opt in
	-a)	eval $optargcmd
		MACHINE_ARCH=$OPTARG; opt_a=yes;;

	-j)	eval $optargcmd
		buildjobs="NBUILDJOBS=$OPTARG";;

	# -m overrides MACHINE_ARCH unless "-a" is specified
	-m)	eval $optargcmd
		MACHINE=$OPTARG; [ "$opt_a" != "yes" ] && getarch;;

	-r)	removedirs=true;;

	-D)	eval $optargcmd
		DESTDIR="$OPTARG";;

	-R)	eval $optargcmd
		releasedir="RELEASEDIR=$OPTARG"; buildtarget=release;;

	-T)	eval $optargcmd
		TOOLDIR="$OPTARG";;

	--)		break;;
	-'?'|-h)	usage;;
esac; done

for var in MACHINE DESTDIR TOOLDIR; do
	if ! eval 'test -n "$'$var'"'; then
		echo "$var must be set in the environment before running build.sh."
		echo ""; usage
	fi
done

# Set up environment.
test -n "$MACHINE_ARCH" || getarch
test -d usr.bin/make || bomb "build.sh must be run from the top source level"

# Remove the target directories.
if ${removedirs-false}; then
	echo "Removing DESTDIR and TOOLDIR...."
	rm -rf $DESTDIR $TOOLDIR
fi

mkdirp $TOOLDIR/bin || bomb "mkdir of $TOOLDIR/bin failed"

# Test make source file timestamps against installed nbmake binary.
if [ -x $TOOLDIR/bin/nbmake ]; then
	for f in usr.bin/make/*.[ch] usr.bin/make/lst.lib/*.[ch]; do
		if [ $f -nt $TOOLDIR/bin/nbmake ]; then
			rebuildmake=true; break
		fi
	done
else
	rebuildmake=true
fi

# Build $TOOLDIR/bin/nbmake.
if ${rebuildmake-false}; then
	echo "Building nbmake...."

	# Go to a temporary directory in case building .o's happens.
	srcdir=`pwd`
	tmpdir=${TMPDIR-/tmp}/nbbuild$$

	mkdir $tmpdir || bomb "cannot mkdir: $tmpdir"
	trap "rm -r -f $tmpdir" 0
	trap "exit 1" 1 2 3 15
	cd $tmpdir

	${HOST_CC-cc} ${HOST_CFLAGS} -DMAKE_BOOTSTRAP \
		-o $TOOLDIR/bin/nbmake -I$srcdir/usr.bin/make \
		$srcdir/usr.bin/make/*.c $srcdir/usr.bin/make/lst.lib/*.c \
		|| bomb "build of nbmake failed"

	# Clean up.
	cd $srcdir
	rm -r -f $tmpdir
	trap 0 1 2 3 15

	# Some compilers are just *that* braindead.
	rm -f $srcdir/usr.bin/make/*.o $srcdir/usr.bin/make/lst.lib/*.o
fi

# Build a nbmake wrapper script, usable by hand as well as by build.sh.
makeprog=$TOOLDIR/bin/nbmake-$MACHINE

if ${rebuildmake-false} || [ ! -f $makeprog ] || [ $makeprog -ot build.sh ]; then
	rm -f $makeprog
	cat >$makeprog <<EOF
#! /bin/sh
# Set proper variables to allow easy "make" building of a NetBSD subtree.
# Generated from:  \$NetBSD: build.sh,v 1.6 2001/10/24 14:55:09 bjh21 Exp $
#
exec $TOOLDIR/bin/nbmake MACHINE=$MACHINE MACHINE_ARCH=$MACHINE_ARCH \
USETOOLS=yes USE_NEW_TOOLCHAIN=yes \${1+\$@}
EOF
	chmod +x $makeprog
fi

exec $makeprog -m `pwd`/share/mk ${buildtarget-build} \
	MKTOOLS=yes DESTDIR="$DESTDIR" TOOLDIR="$TOOLDIR" \
	$buildjobs $releasedir
