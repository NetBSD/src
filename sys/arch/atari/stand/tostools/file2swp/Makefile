#	$NetBSD: Makefile,v 1.1.1.1.2.2 2002/02/28 04:08:31 nathanw Exp $

PROG	= file2swp.ttp

OBJS	= file2swp.o
HEADERS	=
LIBS	= -lxhdi

include ../Makefile.inc

${PROG}: ${OBJS} ${LDADD}
	${CC} -DCOMPRESSED_READ ${LDFLAGS} -o $@ ${OBJS} ${LDADD} ${LIBS}
