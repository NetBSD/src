#	$NetBSD: Makefile,v 1.4 2001/10/31 21:39:03 thorpej Exp $

PROG= bootelf
SRCS= boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CPPFLAGS+= -I${LIBSA}
LDSTATIC?= -static

.include <bsd.prog.mk>
