#	$NetBSD: Makefile,v 1.4 2003/05/18 07:57:32 lukem Exp $

PROG=	mkbootimage
SRCS=	mkbootimage.c byteorder.c
NOMAN=	# defined

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS= -I${LIBSA} -I${.CURDIR}

.include <bsd.prog.mk>
