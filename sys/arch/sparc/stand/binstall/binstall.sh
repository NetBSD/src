#!/bin/sh
#	$NetBSD: binstall.sh,v 1.3 1999/03/01 01:05:51 kim Exp $
#

vecho () {
# echo if VERBOSE on
	if [ "$VERBOSE" = "1" ]; then
		echo "$@" 1>&2
	fi
	return 0
}

Usage () {
	echo "Usage: $0 [-hvt] [-m<path>] net|ffs directory"
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
	echo "	-m<path>	- Look for boot programs in <path> (default: /usr/mdec)"
	echo "	-v		- verbose mode"
	echo "	-t		- test mode (implies -v)"
	exit 0
}

Secure () {
	echo "This script has to be run when the kernel is in 'insecure' mode."
	echo "The best way is to run this script in single-user mode."
	exit 1
}

PATH=/bin:/usr/bin:/sbin:/usr/sbin
MDEC=${MDEC:-/usr/mdec}

if [ "`sysctl -n kern.securelevel`" -gt 0 ]; then
	Secure
fi

set -- `getopt "hm:tv" "$@"`
if [ $? -gt 0 ]; then
	Usage
fi

for a in $*
do
	case $1 in
	-h) Help; shift ;;
	-m) MDEC=$2; shift 2 ;;
	-t) TEST=1; VERBOSE=1; shift ;;
	-v) VERBOSE=1; shift ;;
	--) shift; break ;;
	esac
done

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
	if [ "$DEV" = "" ]; then
		echo "Cannot find \"$DEST\" in mount table"
		exit 1
	fi
	TARGET=$DEST/boot
	vecho Boot device: $DEV
	vecho Target: $TARGET
	$DOIT dd if=${MDEC}/boot of=$TARGET bs=32 skip=$SKIP
	sync; sync; sync
	vecho ${MDEC}/installboot ${VERBOSE:+-v} $TARGET ${MDEC}/bootxx $DEV
	$DOIT ${MDEC}/installboot ${VERBOSE:+-v} $TARGET ${MDEC}/bootxx $DEV
	;;

"net")
	TARGET=$DEST/boot.sparc.netbsd
	vecho Target: $TARGET
	cp -f ${MDEC}/boot.net $TARGET
	;;

*)
	echo "$WHAT: not recognised"
	exit 1
	;;
esac

exit $?
