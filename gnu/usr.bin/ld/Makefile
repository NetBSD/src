#	$NetBSD: Makefile,v 1.17 1998/01/05 21:56:31 cgd Exp $

SUBDIR=	ld ldconfig ldd

.include <bsd.own.mk>	# for NOPIC definition

.if !defined(NOPIC)
SUBDIR+= rtld
.endif

.include <bsd.subdir.mk>
