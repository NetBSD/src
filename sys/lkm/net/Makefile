#	$NetBSD: Makefile,v 1.8 2005/12/11 12:24:49 christos Exp $

.include <bsd.own.mk>

SUBDIR=	bsdcomp deflate tap

.if (${MKPF} != "no")
SUBDIR+= pf
.endif

.include <bsd.subdir.mk>
