#	$NetBSD: Makefile,v 1.2 2000/03/15 11:55:04 fvdl Exp $

DISKLABELPATH=	${.CURDIR}/../../sbin/disklabel
PROG=	mbrlabel
SRCS=	mbrlabel.c dkcksum.c
CPPFLAGS+= -I${DISKLABELPATH}
MAN=	mbrlabel.8
LDADD+=	-lutil

.PATH:	${DISKLABELPATH}
.include <bsd.prog.mk>
