#	$NetBSD: Makefile,v 1.1.14.2 2002/06/20 03:40:43 nathanw Exp $

PROG=	mkbootimage
SRCS=	mkbootimage.c byteorder.c
NOMAN=	# defined

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS= -I${LIBSA} -I${.CURDIR}

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
