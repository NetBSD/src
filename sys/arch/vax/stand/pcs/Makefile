#	$NetBSD: Makefile,v 1.1.2.3 2002/06/20 03:42:17 nathanw Exp $

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
