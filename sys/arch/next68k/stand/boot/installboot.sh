#!/bin/sh
#	$NetBSD: installboot.sh,v 1.2 2002/07/11 16:03:18 christos Exp $

# simple installboot program we can use until we have disklabel to do the job.
# (This one has the advantage that it runs on any architecture. However it
#  expects the bootblock to be located at a very fixed position.)
#
Usage() {
	echo "Usage: installboot bootprog device" >&2
	if [ -n "$1" ]; then echo "$1" >&2; fi
	exit 1
}

if [ $# != 2 ]; then Usage; fi
if [ ! -f $1 ]; then Usage "bootprog must be a regular file"; fi
if [ ! -c $2 ]; then Usage "device must be a character special file"; fi

dd if="$1" of="$2" obs=1024 seek=32 conv=osync
dd if="$1" of="$2" obs=1024 seek=96 conv=osync
exit $?
