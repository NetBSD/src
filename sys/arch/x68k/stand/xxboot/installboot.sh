#! /bin/sh
#
#	do like as disklabel -B
#
#	usage: installboot <boot_file> <boot_device(raw)>
#
#	requires /bin/sh /bin/dd /bin/mkdir /bin/rm /bin/test
#		 /sbin/disklabel /usr/bin/sed /usr/bin/od
#
#	Public domain	--written by Yasha (ITOH Yasufumi)
#
#	$NetBSD: installboot.sh,v 1.1 1998/09/01 20:02:34 itohy Exp $

# disklabel magic (0x82564557) in target byte order
disklabelmagic='202 126 105 127'	# target is Big endian
#disklabelmagic='127 105 126 202'	# target is Little endian
#disklabelmagic='126 202 127 105'	# target is PDP endian

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
isafloppy=false
haslabel=false

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
			isafloppy=true;;
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

# check for disklabel
case "`(dd if=\"$rootdev\" bs=$blksz count=1 | dd bs=4 skip=16 count=1 |
		od -b | sed 1q) 2>/dev/null`" in
'0000000 '*" $disklabelmagic"*)
	$verbose && echo "$rootdev: disklabel exists---may be a floppy disk"
	haslabel=true;;
'0000000  '*|'*')
	if $isafloppy; then
		echo "$rootdev: not a BSD floppy disk"	>&2
		exit 1
	fi
	$verbose && echo "$rootdev: no disklabel in boot block---may be a SCSI disk"
	;;
*)	# error
	if $force; then :; else		# check if not  -f
		echo "$rootdev: can't read boot block"	>&2
		exit 1
	fi
	;;
esac

# write boot block
if $haslabel; then
	trap "rm -rf $temp; exit 1" 1 2 3 15
	rm -rf $temp
	mkdir $temp || exit 1
	bootblk=$temp/bootblk

	# read original disklabel and create new boot block
	# with somewhat paranoid error checkings
	$verbose && echo "extracting disklabel of $rootdev"
	case `( (	dd if="$boot" bs=64 count=1 || echo boot >&3
			(dd if="$rootdev" bs=$blksz count=1 || echo dev >&3) |
				dd bs=4 skip=16 count=69 || echo label >&3
			dd if="$boot" bs=340 skip=1 || echo boot >&3
		) >$bootblk 2>/dev/null ) 3>&1`	in
	'')	;;	# OK
	*dev*)	echo "$rootdev: can't read disklabel"	>&2
		rm -rf $temp; exit 1;;
	*label*)
		echo "can't create temporary file"	>&2
		rm -rf $temp; exit 1;;
	boot*)	echo "$boot: unreadable"		>&2
		rm -rf $temp; exit 1;;
	*)	echo "$boot: unreadable or can't write temporary file"	>&2
		rm -rf $temp; exit 1;;
	esac
	# check if the disklabel is correctly read
	case "`(dd if=$bootblk bs=$blksz count=1 | dd bs=4 skip=16 count=1 |
			od -b | sed 1q) 2>/dev/null`" in
	'0000000 '*" $disklabelmagic"*)	;;
	*)	echo "failed to extract original disklabel"	>&2
		rm -rf $temp; exit 1;;
	esac

	$verbose && echo "writing $boot to $rootdev"
	cmd="dd if=$bootblk bs=$blksz count=$nblock conv=sync,notrunc of=$rootdev"
	$verbose && echo "$cmd"
	if $dowrite; then $cmd; else :; fi
	s=$?
	rm -rf $temp
	exit $s
else
	$verbose && echo "writing $boot to $rootdev"
	cmd="dd if=$boot bs=$blksz count=$nblock conv=sync,notrunc of=$rootdev"
	$verbose && echo "$cmd"
	if $dowrite; then $cmd; else :; fi
fi
