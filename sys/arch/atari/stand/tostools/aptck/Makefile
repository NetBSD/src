#	$NetBSD: Makefile,v 1.3 2002/02/24 20:51:07 leo Exp $

PROG	= aptck.ttp

OBJS	= aptck.o diskio.o disklbl.o
HEADERS	= aptck.h disklbl.h

LIBS	= -lxhdi
CLEAN	= aptck.c

include ../Makefile.inc

aptck.c: aptck.in diskio.c disklbl.c
	${AWK} -f ./setrev.awk $^ > aptck.c

${PROG}: ${OBJS} ${LDADD}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD} ${LIBS}
