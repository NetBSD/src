#	$NetBSD: Makefile,v 1.3 2002/04/25 23:30:26 kleink Exp $

PROG=	mkbootimage
SRCS=	mkbootimage.c byteorder.c
NOMAN=	# defined

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS= -I${LIBSA} -I${.CURDIR}

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
