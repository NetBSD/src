#!/bin/sh
# compatibility with old installboot program
#
# from: @(#)installboot.sh	8.1 (Berkeley) 6/10/93
#
# $Id: installboot.sh,v 1.1 1994/08/04 19:42:18 brezak Exp $
#
if [ $# != 2 ]
then
	echo "Usage: installboot bootprog device"
	exit 1
fi
if [ ! -f $1 ]
then
	echo "Usage: installboot bootprog device"
	echo "${1}: bootprog must be a regular file"
	exit 1
fi
if [ ! -c $2 ]
then
	echo "Usage: installboot bootprog device"
	echo "${2}: device must be a char special file"
	exit 1
fi
/sbin/disklabel -B -b $1 $2
exit $?
