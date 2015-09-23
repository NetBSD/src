#	$NetBSD: Makefile,v 1.1.1.2 2015/09/23 03:03:02 mrg Exp $

SUBDIR+= lib

.include <bsd.own.mk>

.if ${MKGCCCMDS} != "no"
SUBDIR+= .WAIT usr.bin
.endif

.include <bsd.subdir.mk>
