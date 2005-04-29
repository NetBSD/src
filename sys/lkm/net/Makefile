#	$NetBSD: Makefile,v 1.5.4.1 2005/04/29 11:29:28 kent Exp $

.include <bsd.own.mk>

SUBDIR=	bsdcomp deflate tap

.if (${MKPF} != "no")
SUBDIR+= pf
.endif

.include <bsd.subdir.mk>
