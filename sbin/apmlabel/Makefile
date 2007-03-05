#	$NetBSD: Makefile,v 1.1.1.1 2007/03/05 23:06:53 dillo Exp $

.include <bsd.own.mk>

DISKLABELPATH=	${NETBSDSRCDIR}/sbin/disklabel
PROG=	apmlabel
SRCS=	apmlabel.c dkcksum.c printlabel.c
CPPFLAGS+= -I${DISKLABELPATH}
MAN=	apmlabel.8
LDADD+=	-lutil

.PATH:	${DISKLABELPATH} ${NETBSDSRCDIR}/sys/lib/libkern
.include <bsd.prog.mk>
