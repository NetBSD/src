#	$NetBSD: Makefile,v 1.15 1999/02/13 02:55:00 lukem Exp $
#
#       @(#)Makefile	8.15 (Berkeley) 9/21/96

.include <bsd.own.mk>

SUBDIR= src mailstats makemap praliases smrsh

.if ${MKSHARE} != "no"
SUBDIR+= cf/cf
.if make(install)
SUBDIR+= doc/intro doc/op cf
.endif
.endif

.include <bsd.subdir.mk>
