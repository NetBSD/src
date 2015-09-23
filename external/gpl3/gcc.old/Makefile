#	$NetBSD: Makefile,v 1.4 2015/09/23 03:39:07 mrg Exp $

SUBDIR+= lib

.include <bsd.own.mk>

.if ${MKGCCCMDS} != "no"
SUBDIR+= .WAIT usr.bin
.endif

.include <bsd.subdir.mk>
