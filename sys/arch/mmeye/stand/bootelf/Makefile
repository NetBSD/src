#	$NetBSD: Makefile,v 1.3 2001/10/31 01:51:44 thorpej Exp $

PROG= bootelf
SRCS= boot.c loadfile.c loadfile_elf32.c
NOMAN=

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CPPFLAGS+= -I${LIBSA}
LDSTATIC?= -static

.include <bsd.prog.mk>
