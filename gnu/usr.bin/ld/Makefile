#	$NetBSD: Makefile,v 1.18 1998/12/15 22:36:24 pk Exp $

SUBDIR=	ld

.include <bsd.own.mk>	# for NOPIC definition

.if !defined(NOPIC)
SUBDIR+= rtld
.endif

.include <bsd.subdir.mk>
