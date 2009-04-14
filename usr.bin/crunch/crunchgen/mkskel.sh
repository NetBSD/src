#!/bin/sh

#	$NetBSD: mkskel.sh,v 1.3 2009/04/14 22:03:07 lukem Exp $

# idea and sed lines taken straight from flex

cat <<!EOF
/* File created via mkskel.sh */

const char *crunched_skel[] = {
!EOF

sed 's/\\/&&/g' $* | sed 's/"/\\"/g' | sed 's/.*/  "&",/'

cat <<!EOF
  0
};
!EOF
