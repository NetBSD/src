#	$NetBSD: Makefile,v 1.1.1.1.14.2 2002/06/23 17:35:21 jdolecek Exp $

PROG	= file2swp.ttp

OBJS	= file2swp.o
HEADERS	=
LIBS	= -lxhdi

include ../Makefile.inc

${PROG}: ${OBJS} ${LDADD}
	${CC} -DCOMPRESSED_READ ${LDFLAGS} -o $@ ${OBJS} ${LDADD} ${LIBS}
