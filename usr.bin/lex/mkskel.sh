#! /bin/sh
#
#	$NetBSD: mkskel.sh,v 1.3 1998/01/09 08:05:44 perry Exp $
#

cat <<!
/* File created from flex.skl via mkskel.sh */

#include "flexdef.h"

const char *skel[] = {
!

sed 's/\\/&&/g' $* | sed 's/"/\\"/g' | sed 's/.*/  "&",/'

cat <<!
  0
};
!
