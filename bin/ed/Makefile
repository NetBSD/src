#	$NetBSD: Makefile,v 1.30 1999/07/20 09:35:19 mrg Exp $

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
.sinclude "${CRYPTOPATH}/bin/ed/Makefile.frag"
.endif

.include <bsd.prog.mk>
