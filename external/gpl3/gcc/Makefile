#	$NetBSD: Makefile,v 1.1.20.1 2015/01/25 00:19:25 martin Exp $

SUBDIR+= lib

.include <bsd.own.mk>

.if ${MKGCCCMDS} != "no"
SUBDIR+= .WAIT usr.bin
.endif

.include <bsd.subdir.mk>
