#	$NetBSD: Makefile,v 1.2.44.1 2002/03/16 15:56:55 jdolecek Exp $

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
