#!/bin/sh

#	$NetBSD: mkskel.sh,v 1.2 1997/08/02 21:30:13 perry Exp $

# idea and sed lines taken straight from flex

cat <<!EOF
/* File created via mkskel.sh */

char *crunched_skel[] = {
!EOF

sed 's/\\/&&/g' $* | sed 's/"/\\"/g' | sed 's/.*/  "&",/'

cat <<!EOF
  0
};
!EOF
