#	$NetBSD: Makefile,v 1.3.122.1 2009/05/04 08:10:50 yamt Exp $

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
