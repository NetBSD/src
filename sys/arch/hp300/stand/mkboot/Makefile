#	$NetBSD: Makefile,v 1.5 2001/01/02 04:14:34 simonb Exp $

LIBSA=	${.CURDIR}/../../../../lib/libsa

PROG=	mkboot
SRCS=	mkboot.c loadfile.c

.PATH.c: ${LIBSA}

CPPFLAGS+= -I${LIBSA} -I.
LDSTATIC?=-static
MKMAN=	no

.include <bsd.prog.mk>
