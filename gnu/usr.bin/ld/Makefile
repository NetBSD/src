#	$NetBSD: Makefile,v 1.16 1997/10/11 09:34:27 mycroft Exp $

SUBDIR=	ld ldconfig ldd

.include <bsd.own.mk>	# for NOPIC definition

.if !defined(NOPIC)
SUBDIR+= rtld
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
