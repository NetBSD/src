#	$NetBSD: Makefile,v 1.2.4.1 2001/02/26 22:30:59 he Exp $

DISKLABELPATH=	${.CURDIR}/../../sbin/disklabel
PROG=	mbrlabel
SRCS=	mbrlabel.c dkcksum.c printlabel.c
CPPFLAGS+= -I${DISKLABELPATH}
MAN=	mbrlabel.8
LDADD+=	-lutil

.PATH:	${DISKLABELPATH}
.include <bsd.prog.mk>
