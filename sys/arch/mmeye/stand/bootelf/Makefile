#	$NetBSD: Makefile,v 1.2.18.1 2001/11/12 21:17:16 thorpej Exp $

PROG= bootelf
SRCS= boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CPPFLAGS+= -I${LIBSA}
LDSTATIC?= -static

.include <bsd.prog.mk>
