#	$NetBSD: Makefile.inc,v 1.1 2016/01/13 03:39:28 christos Exp $

.include "${.PARSEDIR}/../Makefile.inc"

PROGDPLIBS+= diffutils ${.CURDIR}/../../lib
BINDIR=/usr/bin
