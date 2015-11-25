#	$NetBSD: Makefile,v 1.8 2015/11/25 01:12:03 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>

SECTION=reference/ref1
ARTICLE=mail
SRCS=	mail0.nr mail1.nr mail2.nr mail3.nr mail4.nr mail5.nr mail6.nr \
	mail7.nr mail8.nr mail9.nr maila.nr
MACROS=	-me
.if ${MKREPRO:Uno} == "yes"
MACROS+= -rR=1
.endif
ROFF_TBL=yes
#EXTRAHTMLFILES=# none!

.include <bsd.doc.mk>
