#	$NetBSD: Makefile,v 1.28 1997/10/10 19:56:17 christos Exp $

PROG=	ed
CPPFLAGS+=-DBACKWARDS  #-DDES
SRCS=	buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>
