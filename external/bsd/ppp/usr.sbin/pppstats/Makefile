#	$NetBSD: Makefile,v 1.1.30.1 2020/04/13 07:56:30 martin Exp $

.include "../Makefile.inc"

.PATH: ${DIST}/pppstats

PROG=	pppstats
MAN=	pppstats.8
LINKS=  ${BINDIR}/pppstats ${BINDIR}/slstats
MLINKS=	pppstats.8 slstats.8

COPTS.pppstats.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
