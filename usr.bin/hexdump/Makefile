#	$NetBSD: Makefile,v 1.14.42.1 2020/04/13 08:05:42 martin Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	hexdump
SRCS=	conv.c display.c hexdump.c hexsyntax.c odsyntax.c parse.c
MAN=	hexdump.1 od.1

.ifndef HOSTPROG
LDADD+=-lutil
DPADD+=${LIBUTIL}

LINKS=	${BINDIR}/hexdump ${BINDIR}/od
.endif

COPTS.conv.c += -Wno-format-nonliteral
COPTS.display.c += -Wno-format-nonliteral

COPTS.parse.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
