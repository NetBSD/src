#	$NetBSD: Makefile,v 1.26 1997/07/20 22:36:57 christos Exp $

PROG=	ed
CFLAGS+=-DBACKWARDS  #-DDES
SRCS=	 buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>
