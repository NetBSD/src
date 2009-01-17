#	$NetBSD: Makefile,v 1.3.118.1 2009/01/17 13:27:55 mjf Exp $

PROG	= aptck.ttp

OBJS	= aptck.o diskio.o disklbl.o
HEADERS	= aptck.h disklbl.h

LIBS	= -lxhdi
CLEAN	= aptck.c

include ../Makefile.inc

aptck.c: aptck.in diskio.c disklbl.c
	${TOOL_AWK} -f ./setrev.awk $^ > aptck.c

${PROG}: ${OBJS} ${LDADD}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD} ${LIBS}
