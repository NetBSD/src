#	$NetBSD: Makefile,v 1.4.8.1 2001/01/05 17:34:14 bouyer Exp $

LIBSA=	${.CURDIR}/../../../../lib/libsa

PROG=	mkboot
SRCS=	mkboot.c loadfile.c

.PATH.c: ${LIBSA}

CPPFLAGS+= -I${LIBSA} -I.
LDSTATIC?=-static
MKMAN=	no

.include <bsd.prog.mk>
