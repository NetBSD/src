#! /bin/sh
#
#	size-reduced installboot for the install floppy.
#	supports only SCSI disks.
#
#	requires /bin/sh /bin/dd (x_dd) /bin/mkdir /bin/rm /bin/test
#		 /sbin/disklabel
#
#	$NetBSD: installboot.sh,v 1.1 2002/05/07 13:55:43 isaki Exp $
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

PATH=/bin:/sbin:/usr/bin

echo "$0: THIS VERSION OF $0 DOES NO ERROR CHECK!"
echo "$0: Sleeping for 5 seconds; press ^C to abort."
echo "===> cat $1 /dev/zero | dd ibs=1 count=8192 obs=512 of=$2"
sleep 5
cat $1 /dev/zero | dd ibs=1 count=8192 obs=512 of=$2
