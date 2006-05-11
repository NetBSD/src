#	$NetBSD: Makefile,v 1.33 2006/05/11 23:16:28 mrg Exp $

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

.if ${HAVE_GCC} == 4
COPTS.cbc.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
