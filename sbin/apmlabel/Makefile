#	$NetBSD: Makefile,v 1.2 2012/03/21 00:35:33 riz Exp $

.include <bsd.own.mk>

DISKLABELPATH=	${NETBSDSRCDIR}/sbin/disklabel
PROG=	apmlabel
SRCS=	apmlabel.c dkcksum.c printlabel.c
CPPFLAGS+= -I${DISKLABELPATH}
MAN=	apmlabel.8
LDADD+=	-lutil

.PATH:	${DISKLABELPATH}
.include <bsd.prog.mk>
