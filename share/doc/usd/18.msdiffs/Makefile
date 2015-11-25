#	$NetBSD: Makefile,v 1.6 2015/11/25 01:20:11 christos Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>
SECTION=reference/ref1/roff
ARTICLE=msdiffs
SRCS=	ms.diffs
MACROS=	-ms
.if ${MKREPRO:Uno} == "yes"
MACROS+= -rR=1
.endif

EXTRAHTMLFILES=msdiffs1.png msdiffs2.png

.include <bsd.doc.mk>
