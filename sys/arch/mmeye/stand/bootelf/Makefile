#	$NetBSD: Makefile,v 1.5 2001/12/12 01:49:46 tv Exp $

PROG= bootelf
SRCS= boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=
# defined
LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CPPFLAGS+= -I${LIBSA}
LDSTATIC?= -static

.include <bsd.prog.mk>
