#	$NetBSD: Makefile,v 1.1 1998/11/16 18:44:25 ws Exp $

DISKLABELPATH=	${.CURDIR}/../../sbin/disklabel
PROG=	mbrlabel
SRCS=	mbrlabel.c dkcksum.c
CPPFLAGS+= -I${DISKLABELPATH}
MAN=	mbrlabel.8
LDADD+=	-lutil

.PATH:	${DISKLABELPATH}
.include <bsd.prog.mk>
