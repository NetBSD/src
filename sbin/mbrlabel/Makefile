#	$NetBSD: Makefile,v 1.4 2002/08/19 10:16:55 lukem Exp $

.include <bsd.own.mk>

DISKLABELPATH=	${NETBSDSRCDIR}/sbin/disklabel
PROG=	mbrlabel
SRCS=	mbrlabel.c dkcksum.c printlabel.c
CPPFLAGS+= -I${DISKLABELPATH}
MAN=	mbrlabel.8
LDADD+=	-lutil

.PATH:	${DISKLABELPATH}
.include <bsd.prog.mk>
