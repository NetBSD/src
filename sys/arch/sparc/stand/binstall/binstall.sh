#!/bin/sh
#	$NetBSD: binstall.sh,v 1.4.4.2 2002/03/20 22:21:23 he Exp $
#

vecho () {
# echo if VERBOSE on
	if [ "$VERBOSE" = "1" ]; then
		echo "$@" 1>&2
	fi
	return 0
}

Usage () {
	echo "Usage: $0 [-hvtuU] [-m<path>] net|ffs directory"
	exit 1
}

Help () {
	echo "This script copies the boot programs to one of several"
	echo "commonly used places. It takes care of stripping the"
	echo "a.out(5) header off the installed boot program on sun4 machines."
	echo "When installing an \"ffs\" boot program, this script also runs"
	echo "installboot(8) which installs the default proto bootblocks into"
	echo "the appropriate filesystem partition."
	echo "Options:"
	echo "	-h		- display this message"
	echo "	-u		- install sparc64 (UltraSPARC) boot block"
	echo "	-U		- install sparc boot block"
	echo "	-b<bootprog>	- second-stage boot program to install"
	echo "  -f<path name>	- path to device/file image for filesystem"
	echo "	-m<path>	- Look for boot programs in <path> (default: /usr/mdec)"
	echo "	-v		- verbose mode"
	echo "	-t		- test mode (implies -v)"
	exit 0
}

Secure () {
	echo "This script has to be run when the kernel is in 'insecure' mode,"
	echo "or when applying bootblocks to a file image (ala vnd)."
	echo "The best way is to run this script in single-user mode."
	exit 1
}

PATH=/bin:/usr/bin:/sbin:/usr/sbin
MDEC=${MDEC:-/usr/mdec}
BOOTPROG=${BOOTPROG:-boot}
OFWBOOT=${OFWBOOTBLK:-ofwboot}
if [ "`sysctl -n hw.machine`" = sparc64 ]; then
	ULTRASPARC=1
else
	ULTRASPARC=0
fi

set -- `getopt "b:hm:f:tvuU" "$@"`
if [ $? -gt 0 ]; then
	Usage
fi

for a in $*
do
	case $1 in
	-h) Help; shift ;;
	-u) ULTRASPARC=1; shift ;;
	-U) ULTRASPARC=0; shift ;;
	-b) BOOTPROG=$2; OFWBOOT=$2; shift 2 ;;
	-f) FILENAME=$2; shift 2 ;;
	-m) MDEC=$2; shift 2 ;;
	-t) TEST=1; VERBOSE=1; shift ;;
	-v) VERBOSE=1; shift ;;
	--) shift; break ;;
	esac
done

if [ "`sysctl -n kern.securelevel`" -gt 0 ] && [ ! -f "$FILENAME" ]; then
	Secure
fi

DOIT=${TEST:+echo "=>"}

if [ $# != 2 ]; then
	Usage
fi

WHAT=$1
DEST=$2

if [ ! -d $DEST ]; then
	echo "$DEST: not a directory"
	Usage
fi

SKIP=0

if [ "$ULTRASPARC" = "1" ]; then
	targ=ofwboot
	netboot=ofwboot.net
	nettarg=boot.sparc.netbsd
	BOOTPROG=$OFWBOOT
else
	targ=boot
	netboot=boot.net
	nettarg=boot.sparc64.netbsd
fi

case $WHAT in
"ffs")
	DEV=`mount | while read line; do
		set -- $line
		vecho "Inspecting \"$line\""
		if [ "$2" = "on" -a "$3" = "$DEST" ]; then
			if [ ! -b $1 ]; then
				continue
			fi
			RAW=\`echo -n "$1" | sed -e 's;/dev/;/dev/r;'\`
			if [ ! -c \$RAW ]; then
				continue
			fi
			echo -n $RAW
			break;
		fi
	done`
	if [ "$FILENAME" != "" ]; then
		DEV=$FILENAME
	fi
	if [ "$DEV" = "" ]; then
		echo "Cannot find \"$DEST\" in mount table"
		exit 1
	fi
	TARGET=$DEST/$targ
	vecho Boot device: $DEV
	vecho Target: $TARGET
	$DOIT dd if=${MDEC}/${BOOTPROG} of=$TARGET bs=32 skip=$SKIP
	sync; sync; sync
	if [ "$ULTRASPARC" = "1" ]; then
		vecho ${MDEC}/installboot -u ${VERBOSE:+-v} ${MDEC}/bootblk $DEV
		$DOIT ${MDEC}/installboot -u ${VERBOSE:+-v} ${MDEC}/bootblk $DEV
	else
		vecho ${MDEC}/installboot ${VERBOSE:+-v} $TARGET ${MDEC}/bootxx $DEV
		$DOIT ${MDEC}/installboot ${VERBOSE:+-v} $TARGET ${MDEC}/bootxx $DEV
	fi
	;;

"net")
	TARGET=$DEST/$nettarg
	vecho Target: $TARGET
	$DOIT cp -f ${MDEC}/$netboot $TARGET
	;;

*)
	echo "$WHAT: not recognised"
	exit 1
	;;
esac

exit $?
