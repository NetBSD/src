#	$NetBSD: Makefile,v 1.7 2005/02/22 14:40:01 peter Exp $

.include <bsd.own.mk>

SUBDIR=	bsdcomp deflate tap

.if (${MKPF} != "no")
SUBDIR+= pf
.endif

.include <bsd.subdir.mk>
