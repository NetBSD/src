#	$NetBSD: Makefile,v 1.10 2006/08/26 18:17:42 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	hexdump
SRCS=	conv.c display.c hexdump.c hexsyntax.c odsyntax.c parse.c
MAN=	hexdump.1 od.1
WFORMAT=	1
WARNS=		2

LDADD+=-lutil
DPADD+=${LIBUTIL}

.ifndef HOSTPROG
LINKS=	${BINDIR}/hexdump ${BINDIR}/od
.endif

.include <bsd.prog.mk>
