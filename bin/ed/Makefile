#	$NetBSD: Makefile,v 1.25 1997/07/20 17:22:15 christos Exp $

WARNS=1
PROG=	ed
CFLAGS+=-DBACKWARDS  #-DDES
SRCS=	 buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>
