#	$NetBSD: Makefile,v 1.14 1998/09/29 07:03:37 lukem Exp $
#
#       @(#)Makefile	8.15 (Berkeley) 9/21/96

SUBDIR= src mailstats makemap praliases smrsh

.if !defined(NOSHARE)
SUBDIR+= cf/cf
.if make(install)
SUBDIR+= doc/intro doc/op cf
.endif
.endif

.include <bsd.subdir.mk>
