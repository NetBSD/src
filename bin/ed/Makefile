#	$NetBSD: Makefile,v 1.30.6.1 2000/06/22 15:03:43 minoura Exp $

PROG=	ed
CPPFLAGS+=-DBACKWARDS
CPPFLAGS+=-DDES
SRCS=	buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>
