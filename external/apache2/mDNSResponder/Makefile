#	$NetBSD: Makefile,v 1.3 2009/11/28 22:15:48 he Exp $

.include <bsd.own.mk>

SUBDIR=	usr.bin usr.sbin
.if (${MKPIC} != "no")
SUBDIR+= nss
.endif

.include <bsd.subdir.mk>
