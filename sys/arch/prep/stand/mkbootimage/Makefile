#	$NetBSD: Makefile,v 1.1.10.2 2002/06/23 17:39:56 jdolecek Exp $

PROG=	mkbootimage
SRCS=	mkbootimage.c byteorder.c
NOMAN=	# defined

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS= -I${LIBSA} -I${.CURDIR}

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
