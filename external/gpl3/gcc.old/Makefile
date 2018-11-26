#	$NetBSD: Makefile,v 1.6.2.1 2018/11/26 01:51:16 pgoyette Exp $

SUBDIR+= lib

.include <bsd.own.mk>

.if ${MKGCCCMDS} != "no"
SUBDIR+= .WAIT usr.bin
.endif

.include <bsd.subdir.mk>
