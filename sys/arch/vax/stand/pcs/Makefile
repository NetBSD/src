#	$NetBSD: Makefile,v 1.2 2002/04/24 08:18:53 lukem Exp $

NOMAN=

.include <bsd.own.mk>

FILES=		pcs750.bin
FILESDIR=	/
FILESMODE=	${BINMODE}
CLEANFILES+=	pcs750.bin

all dependall: ${FILES}

UUDECODE?=	uudecode

pcs750.bin:	${.CURDIR}/pcs750.bin.uue
	${UUDECODE} ${.ALLSRC}

.include <bsd.prog.mk>
