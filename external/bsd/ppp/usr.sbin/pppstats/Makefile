#	$NetBSD: Makefile,v 1.3 2023/06/03 09:09:05 lukem Exp $

.include "../Makefile.inc"

.PATH: ${DIST}/pppstats

PROG=	pppstats
MAN=	pppstats.8
LINKS=  ${BINDIR}/pppstats ${BINDIR}/slstats
MLINKS=	pppstats.8 slstats.8

COPTS.pppstats.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
