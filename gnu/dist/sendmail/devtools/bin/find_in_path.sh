#! /bin/sh
#
# Id: find_in_path.sh,v 8.2 1999/09/23 20:42:22 gshapiro Exp
# $NetBSD: find_in_path.sh,v 1.1.1.3 2003/06/01 14:01:14 atatat Exp $
#
EX_OK=0
EX_NOT_FOUND=1

ifs="$IFS"; IFS="${IFS}:"
for dir in $PATH /usr/5bin /usr/ccs/bin
do
	if [ -r $dir/$1 ]
	then
		echo $dir/$1
		exit $EX_OK
	fi
done
IFS=$ifs

exit $EX_NOT_FOUND
