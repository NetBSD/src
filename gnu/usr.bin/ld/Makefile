#	$NetBSD: Makefile,v 1.15 1997/04/16 16:49:28 christos Exp $

SUBDIR=	ld ldconfig ldd

.include <bsd.own.mk>	# for NOPIC definition

.if !defined(NOPIC)
SUBDIR+= rtld
.endif

.include <bsd.prog.mk>
