#	$NetBSD: Makefile,v 1.3 2000/12/24 13:57:37 lukem Exp $

DISKLABELPATH=	${.CURDIR}/../../sbin/disklabel
PROG=	mbrlabel
SRCS=	mbrlabel.c dkcksum.c printlabel.c
CPPFLAGS+= -I${DISKLABELPATH}
MAN=	mbrlabel.8
LDADD+=	-lutil

.PATH:	${DISKLABELPATH}
.include <bsd.prog.mk>
