#	$NetBSD: Makefile,v 1.35 2008/08/29 00:02:21 gmcgarry Exp $

.include <bsd.own.mk>

PROG=	ed
CPPFLAGS+=-DBACKWARDS

.if (${MKCRYPTO} != "no")
CPPFLAGS+=-DDES
.endif

SRCS=	buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

#LINKS=  ${BINDIR}/ed ${BINDIR}/red
#MLINKS= ed.1 red.1

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.cbc.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
