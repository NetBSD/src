#!/bin/sh
#	$NetBSD: binstall.sh,v 1.18 2018/09/22 03:29:51 kre Exp $
#

vecho () {
	# echo if VERBOSE on
	[ -n "$VERBOSE" ] && echo "$*" 1>&2
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
	echo "Usage: $0"' [options] <"net"|"ffs"> <directory>'
	Options
	exit 1
}

Help () {
	echo "This script copies the boot programs to one of several"
	echo "commonly used places."
	echo 'When installing an "ffs" boot program, this script also runs'
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
VERBOSE=
: ${MDEC:=/usr/mdec}
: ${INSTALLBOOT:=/usr/sbin/installboot}
: ${BOOTPROG:=boot}
: ${OFWBOOTBLK:=ofwboot}
[ "$( sysctl -n machdep.cpu_arch )" = 9 ] && ULTRASPARC=true || ULTRASPARC=false

while getopts b:hf:i:m:tUuv a
do
	case "$a" in
	h) Help;;
	u) ULTRASPARC=true;;
	U) ULTRASPARC=false;;
	b) BOOTPROG=$OPTARG; OFWBOOTBLK=$OPTARG;;
	f) DEV=$OPTARG;;
	m) MDEC=$OPTARG;;
	i) INSTALLBOOT=$OPTARG;;
	t) TEST=1; VERBOSE=-v;;
	v) VERBOSE=-v;;
	\?) Usage;;
	esac
done
shift $(( $OPTIND - 1 ))

if [ "$( sysctl -n kern.securelevel )" -gt 0 ] && ! [ -f "$DEV" ]; then
	Secure
fi

DOIT=${TEST:+echo "=>"}

if [ $# != 2 ]; then
	Usage
fi

WHAT=$1
DEST=$2

if ! [ -d "$DEST" ]; then
	echo "$DEST: not a directory"
	Usage
fi

if $ULTRASPARC
then
	machine=sparc64
	targ=ofwboot
	stage2=""
	netboot=ofwboot
	BOOTPROG=$OFWBOOTBLK
	BOOTXX=${MDEC}/bootblk
else
	machine=sparc
	targ=boot
	stage2=${targ}
	netboot=boot.net
	BOOTXX=${MDEC}/bootxx
fi

case $WHAT in
"ffs")
	if [ -z "$DEV" ]; then
		# Lookup device mounted on DEST
		DEV=$( mount | while read line; do
			set -- $line
			vecho "Inspecting \"$line\""
			if [ "$2" = "on" ] && [ "$3" = "$DEST" ]; then
				[ -b "$1" ] || continue
				case "$1" in
				(*/*) RAW="${1%/*}/r${1##*/}";;
				(*)   RAW="r$1";;
				esac
				[ -c "$RAW" ] || continue
				echo -n "$RAW"
				break;
			fi
		done )
		if [ -z "$DEV" ]; then
			echo "Cannot find \"$DEST\" in mount table"
			exit 1
		fi
	fi

	vecho "Boot device: $DEV"
	vecho "Primary boot program: $BOOTXX"
	vecho "Secondary boot program: $DEST/$targ"

	$DOIT cp -p -f "${MDEC}/${BOOTPROG}" "$DEST/$targ"
	sync; sync; sync
	vecho "${INSTALLBOOT} ${VERBOSE} -m $machine $DEV $BOOTXX $stage2"
	$DOIT "${INSTALLBOOT}" ${VERBOSE} -m "$machine" \
		"$DEV" "$BOOTXX" "$stage2"
	;;

"net")
	vecho "Network boot program: $DEST/$boot.${machine}.netbsd"
	$DOIT cp -p -f "${MDEC}/$netboot" "$DEST/$boot.${machine}.netbsd"
	;;

*)
	echo "$WHAT: not recognised"
	exit 1
	;;
esac

exit $?
