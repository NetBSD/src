#	$NetBSD: Makefile,v 1.1 2002/04/09 18:03:07 matt Exp $

.include <bsd.own.mk>

FILES=		pcs750.bin
FILESDIR=	/
FILESMODE=	${BINMODE}
CLEANFILES+=	pcs750.bin

all dependall: ${FILES}

UUDECODE?=	uudecode

pcs750.bin:	${.CURDIR}/pcs750.bin.uue
	${UUDECODE} ${.ALLSRC}

.include <bsd.files.mk>

NOMAN=

.include <bsd.prog.mk>
