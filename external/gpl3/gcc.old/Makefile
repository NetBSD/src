#	$NetBSD: Makefile,v 1.6 2017/07/23 01:10:50 mrg Exp $

SUBDIR+= lib

.include <bsd.own.mk>

.if ${MKGCCCMDS} != "no"
SUBDIR+= .WAIT usr.bin
.endif

.include <bsd.subdir.mk>
