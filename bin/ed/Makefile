#	$NetBSD: Makefile,v 1.29 1999/07/12 22:04:09 thorpej Exp $

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	ed
CPPFLAGS+=-DBACKWARDS
SRCS=	buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.if defined(CRYPTOPATH)
.include "${CRYPTOPATH}/bin/ed/Makefile.frag"
.endif

.include <bsd.prog.mk>
