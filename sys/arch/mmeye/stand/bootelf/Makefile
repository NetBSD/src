#	$NetBSD: Makefile,v 1.2 1999/09/15 09:47:19 tsubai Exp $

PROG= bootelf
SRCS= boot.c loadfile.c
NOMAN=

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CPPFLAGS+= -I${LIBSA}
LDSTATIC?= -static

.include <bsd.prog.mk>
