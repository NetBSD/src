#	$NetBSD: Makefile,v 1.2.6.2 2002/06/23 17:43:03 jdolecek Exp $

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
