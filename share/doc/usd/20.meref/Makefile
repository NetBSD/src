#	$NetBSD: Makefile,v 1.5 2015/11/25 01:20:11 christos Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>
SECTION=reference/ref1/roff
ARTICLE=meref
SRCS=	ref.me
MACROS=	-me
.if ${MKREPRO:Uno} == "yes"
MACROS+= -rR=1
.endif

.include <bsd.doc.mk>
