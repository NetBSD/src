#!/bin/sh
#	$NetBSD: binstall.sh,v 1.11 2002/05/07 14:13:02 pk Exp $
#

vecho () {
# echo if VERBOSE on
	if [ "$VERBOSE" = "1" ]; then
		echo "$@" 1>&2
	fi
	return 0
}

Options () {
	echo "Options:"
	echo "	-h		- display this message"
	echo "	-u		- install sparc64 (UltraSPARC) boot block"
	echo "	-U		- install sparc boot block"
	echo "	-b<bootprog>	- second-stage boot program to install"
	echo "	-f<pathname>	- path to device/file image for filesystem"
	echo "	-m<path>	- Look for boot programs in <path> (default: /usr/mdec)"
	echo "	-i<progname>	- Use the installboot program at <progname>"
	echo "			  (default: /usr/sbin/installboot)"
	echo "	-v		- verbose mode"
	echo "	-t		- test mode (implies -v)"
}

Usage () {
	echo "Usage: $0 [options] <"'"net"|"ffs"'"> <directory>"
	Options
	exit 1
}

Help () {
	echo "This script copies the boot programs to one of several"
	echo "commonly used places."
	echo "When installing an \"ffs\" boot program, this script also runs"
	echo "installboot(8) which installs the default proto bootblocks into"
	echo "the appropriate filesystem partition or filesystem image."
	Options
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
INSTALLBOOT=${INSTALLBOOT:=/usr/sbin/installboot}
BOOTPROG=${BOOTPROG:-boot}
OFWBOOT=${OFWBOOTBLK:-ofwboot}
if [ "`sysctl -n hw.machine`" = sparc64 ]; then
	ULTRASPARC=1
else
	ULTRASPARC=0
fi

set -- `getopt "b:hf:i:m:tUuv" "$@"`
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
	-f) DEV=$2; shift 2 ;;
	-m) MDEC=$2; shift 2 ;;
	-i) INSTALLBOOT=$2; shift 2 ;;
	-t) TEST=1; VERBOSE=1; shift ;;
	-v) VERBOSE=1; shift ;;
	--) shift; break ;;
	esac
done

if [ "`sysctl -n kern.securelevel`" -gt 0 ] && [ ! -f "$DEV" ]; then
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

if [ "$ULTRASPARC" = "1" ]; then
	machine=sparc64
	targ=ofwboot
	netboot=ofwboot.net
	BOOTPROG=$OFWBOOT
	BOOTXX=${MDEC}/bootblk
else
	machine=sparc
	targ=boot
	netboot=boot.net
	BOOTXX=${MDEC}/bootxx
fi

case $WHAT in
"ffs")
	if [ "$DEV" = "" ]; then
		# Lookup device mounted on DEST
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
		if [ "$DEV" = "" ]; then
			echo "Cannot find \"$DEST\" in mount table"
			exit 1
		fi
	fi

	vecho Boot device: $DEV
	vecho Primary boot program: $BOOTXX
	vecho Secondary boot program: $DEST/$targ

	$DOIT cp -p -f ${MDEC}/${BOOTPROG} $DEST/$targ
	sync; sync; sync
	vecho ${INSTALLBOOT} ${VERBOSE:+-v} -m $machine $DEV ${BOOTXX} $targ
	$DOIT ${INSTALLBOOT} ${VERBOSE:+-v} -m $machine $DEV ${BOOTXX} $targ
	;;

"net")
	vecho Network boot program: $DEST/$boot.${machine}.netbsd
	$DOIT cp -p -f ${MDEC}/$netboot $DEST/$boot.${machine}.netbsd
	;;

*)
	echo "$WHAT: not recognised"
	exit 1
	;;
esac

exit $?
