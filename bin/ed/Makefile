#	$NetBSD: Makefile,v 1.27 1997/08/03 04:27:37 mikel Exp $

PROG=	ed
CFLAGS+=-DBACKWARDS  #-DDES
SRCS=	buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>
