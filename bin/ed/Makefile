#	$NetBSD: Makefile,v 1.31 2000/06/16 16:33:06 thorpej Exp $

PROG=	ed
CPPFLAGS+=-DBACKWARDS
CPPFLAGS+=-DDES
SRCS=	buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.include <bsd.prog.mk>
