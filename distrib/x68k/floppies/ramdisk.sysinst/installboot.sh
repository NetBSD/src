#! /bin/sh
#
#	size-reduced installboot for the install floppy.
#	supports only SCSI disks.
#
#	requires /bin/sh /bin/dd (x_dd) /bin/mkdir /bin/rm /bin/test
#		 /sbin/disklabel
#
#	$NetBSD: installboot.sh,v 1.3 2000/06/18 10:33:20 minoura Exp $
#
#
#    originally from:
#	do like as disklabel -B
#
#	usage: installboot <boot_file> <boot_device(raw)>
#
#	requires /bin/sh /bin/dd /bin/mkdir /bin/rm /bin/test
#		 /sbin/disklabel /usr/bin/sed /usr/bin/od
#
#	Public domain	--written by Yasha (ITOH Yasufumi)
#
#	NetBSD: installboot.sh,v 1.1 1998/09/01 20:02:34 itohy Exp

usage='echo "usage: $0 [-nvf] /usr/mdec/xxboot /dev/rxx?a"	>&2; exit 1'

blksz=1024
nblock=8	# boot block in $blksz (8KB)
rawpart=c

PATH=/bin:/sbin:/usr/bin

# parse options
dowrite=true
verbose=false
force=false
while (case "$1" in -*) ;; *) exit 1;; esac); do
	case "$1" in
	-[nvf]|-[nvf][nvf]|-[nvf][nvf][nvf]|-[nvf][nvf][nvf][nvf]) # enough?
		case "$1" in *n*) dowrite=false;; esac
		case "$1" in *v*) verbose=true;; esac
		case "$1" in *f*) force=true;; esac
		shift
		;;
	*)
		eval $usage;;
	esac
done

case "$#" in
2)	;;
*)	eval $usage;;
esac

boot="$1"
rootdev="$2"
temp=/tmp/installboot$$

# report error early
if test ! -f "$boot"; then
	echo "$boot: file not found"	>&2
	eval $usage
fi

# check device so as not to destroy non-BSD partitions
if test -b "$rootdev"; then
	echo "$rootdev: is a block special---specify a char special"
	exit 1
fi
if test -c "$rootdev"; then
	if $force; then :; else		# check if not  -f
		case "$rootdev" in
		/dev/rfd??)	# floppies---check disklabel later
			echo "$rootdev: floppies are not supported!"
			exit 1
		/dev/r*[a-h])	# SCSI disks
			$verbose && echo "checking partition type..."
			rawdev="`echo \"$rootdev\" | sed 's/.$/'$rawpart'/'`"
			part="`echo \"$rootdev\" | sed 's/^.*\(.\)$/\1/'`"
			pinfo="`disklabel \"$rawdev\" | sed '1,/^$/d' |
				sed -n -e 's/^#.*/#/p' -e \"/^  $part/p\"`"
			$verbose && echo "$pinfo" | sed '/^#/d'
			case "$pinfo" in
			'#'*' '$part:*4.2BSD*)
				$verbose && echo "partition OK"
				;;
			'#')	echo "$rootdev: no such partition"	>&2
				exit 1;;
			'')	echo "$rootdev: can't read disklabel"	>&2
				exit 1;;
			*)	echo "$rootdev: not a BSD filesystem"	>&2
				exit 1;;
			esac
			;;
		/*)		# ???
			echo "$rootdev: can't install boot to this device" >&2
			exit 1;;
		*)
			echo "$rootdev: not in the absolute path"	>&2
			exit 1;;
		esac
	fi
elif test ! -f "$rootdev"; then
	echo "$rootdev: no such file or character special"	>&2
	exit 1
fi

# write boot block
$verbose && echo "writing $boot to $rootdev"
cmd="cat $boot /dev/null dd bs=$blksz count=$nblock of=$rootdev"
$verbose && echo "$cmd"
if $dowrite; then $cmd; else :; fi
