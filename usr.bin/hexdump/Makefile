#	$NetBSD: Makefile,v 1.8 2003/03/07 01:14:31 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	hexdump
SRCS=	conv.c display.c hexdump.c hexsyntax.c odsyntax.c parse.c
MAN=	hexdump.1 od.1
WFORMAT=	1
WARNS=		2

.ifndef HOSTPROG
LINKS=	${BINDIR}/hexdump ${BINDIR}/od
.include <bsd.prog.mk>
.endif
