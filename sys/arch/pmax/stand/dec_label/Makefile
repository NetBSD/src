#	$NetBSD: Makefile,v 1.1 1997/05/25 21:39:13 jonathan Exp $

S=	${.CURDIR}/../../../../../

INCPATH=-I${.CURDIR} -I${.CURDIR}/../../.. -I${S} -I${S}/lib/libsa

CFLAGS=	-O2 ${INCPATH} ${DEFS}


PROG= dec_label
NOMAN= 1
BINDIR=/usr/sbin

SRCS= dec_label.c
.include <bsd.prog.mk>
