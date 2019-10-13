#	$NetBSD: Makefile,v 1.2 2019/10/13 07:28:07 mrg Exp $

.include "../Makefile.inc"

.PATH: ${DIST}/pppstats

PROG=	pppstats
MAN=	pppstats.8
LINKS=  ${BINDIR}/pppstats ${BINDIR}/slstats
MLINKS=	pppstats.8 slstats.8

COPTS.pppstats.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
