#	$NetBSD: Makefile,v 1.7 2018/11/11 22:49:37 mrg Exp $

SUBDIR+= lib

.include <bsd.own.mk>

.if ${MKGCCCMDS} != "no"
SUBDIR+= .WAIT usr.bin
.endif

.include <bsd.subdir.mk>
