#	$NetBSD: Makefile,v 1.6.2.1 2005/03/19 08:36:29 yamt Exp $

.include <bsd.own.mk>

SUBDIR=	bsdcomp deflate tap

.if (${MKPF} != "no")
SUBDIR+= pf
.endif

.include <bsd.subdir.mk>
