#! /bin/sh
#  $NetBSD: build.sh,v 1.1 2001/10/19 02:21:03 tv Exp $
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
		acorn32|cats|dnard|evbarm|hpcarm|netwinder)
			MACHINE_ARCH=arm;;

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

		*)	MACHINE_ARCH=$MACHINE;;
	esac
}

usage () {
	echo "Usage:"
	echo "$0 [-nr] [-m machine] [-D destdir] [-T tooldir]"
	echo "    -m: set target MACHINE to machine"
	echo "    -n: do not build a release (just install to DESTDIR)"
	echo "    -r: remove TOOLDIR and DESTDIR before the build"
	echo "    -D: set DESTDIR to destdir"
	echo "    -T: set TOOLDIR to tooldir"
	exit 1
}

while getopts m:nrD:T: opt; do case $opt in
	m)	MACHINE=$OPTARG; getarch;; # getarch overrides MACHINE_ARCH

	n)	buildtarget=build;;

	r)	removedirs=true;;

	D)	DESTDIR="$OPTARG";;

	T)	TOOLDIR="$OPTARG";;

	'?')	usage;;
esac; done

for var in DESTDIR TOOLDIR MACHINE; do
	eval 'test -n "$'$var'"' || \
		bomb "$var must be set in the environment before running build.sh"
done

# Set up environment.
test -n "$MACHINE_ARCH" || getarch
test -d usr.bin/make || bomb "build.sh must be run from the top source level"
[ -d $TOOLDIR/bin ] || mkdir -p $TOOLDIR/bin || bomb "mkdir of $TOOLDIR/bin failed"

# Remove the target directories.
if ${removedirs-false}; then
	echo "Removing DESTDIR and TOOLDIR...."
	rm -rf $DESTDIR $TOOLDIR
fi

# Test make source file timestamps against installed bmake binary.
if [ -x $TOOLDIR/bin/bmake ]; then
	for f in usr.bin/make/*.[ch] usr.bin/make/lst.lib/*.[ch]; do
		if [ $f -nt $TOOLDIR/bin/bmake ]; then
			rebuildmake=true; break
		fi
	done
else
	rebuildmake=true
fi

# Build $TOOLDIR/bin/bmake.
if ${rebuildmake-false}; then
	echo "Building bmake...."

	# Go to a temporary directory in case building .o's happens.
	srcdir=`pwd`
	cd ${TMPDIR-/tmp}

	${HOST_CC-cc} ${HOST_CFLAGS} -DMAKE_BOOTSTRAP \
		-o $TOOLDIR/bin/bmake -I$srcdir/usr.bin/make \
		$srcdir/usr.bin/make/*.c $srcdir/usr.bin/make/lst.lib/*.c

	# Clean up.
	rm -f *.o
	cd $srcdir

	# Some compilers are just *that* braindead.
	rm -f $srcdir/usr.bin/make/*.o $srcdir/usr.bin/make/lst.lib/*.o
fi

$TOOLDIR/bin/bmake ${buildtarget-release} -m `pwd`/share/mk \
	MKTOOLS=yes DESTDIR="$DESTDIR" TOOLDIR="$TOOLDIR" \
	MACHINE=$MACHINE MACHINE_ARCH=$MACHINE_ARCH
