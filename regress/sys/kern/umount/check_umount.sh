#!/bin/sh
# $NetBSD: check_umount.sh,v 1.1 2003/04/15 06:19:59 erh Exp $

# This script expects to be run with the current directory
# set to a location that has just been forcibly unmounted.

retval=0

checkit()
{
	OUTPUT=`$CMD 2>&1`
	if [ "$OUTPUT" != "$EXPECT" ] ; then
		retval=1
		echo "Command \"$CMD\" failed:"
		echo "OUTPUT: $OUTPUT"
		echo "EXPECTED: $EXPECT"
	fi
}

CMD="ls ."
EXPECT="ls: .: No such file or directory"
checkit
CMD="ls .."
EXPECT="ls: ..: No such file or directory"
checkit
CMD="cd ."
EXPECT="cd: can't cd to ."
checkit
CMD="cd .."
EXPECT="cd: can't cd to .."
checkit

exit $retval
