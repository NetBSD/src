#	$NetBSD: Makefile,v 1.2.16.1 2002/01/10 19:46:29 thorpej Exp $

PROG= bootelf
SRCS= boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=
# defined
LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CPPFLAGS+= -I${LIBSA}
LDSTATIC?= -static

.include <bsd.prog.mk>
