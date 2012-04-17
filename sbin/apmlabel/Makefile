#	$NetBSD: Makefile,v 1.1.1.1.36.1 2012/04/17 00:05:38 yamt Exp $

.include <bsd.own.mk>

DISKLABELPATH=	${NETBSDSRCDIR}/sbin/disklabel
PROG=	apmlabel
SRCS=	apmlabel.c dkcksum.c printlabel.c
CPPFLAGS+= -I${DISKLABELPATH}
MAN=	apmlabel.8
LDADD+=	-lutil

.PATH:	${DISKLABELPATH}
.include <bsd.prog.mk>
