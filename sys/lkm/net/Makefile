#	$NetBSD: Makefile,v 1.1.2.5 2005/03/04 16:52:48 skrll Exp $

.include <bsd.own.mk>

SUBDIR=	bsdcomp deflate tap

.if (${MKPF} != "no")
SUBDIR+= pf
.endif

.include <bsd.subdir.mk>
