#	$NetBSD: Makefile,v 1.3 2003/07/10 10:34:41 lukem Exp $

NOMAN=

.include <bsd.own.mk>

FILES=		pcs750.bin
FILESDIR=	/
FILESMODE=	${BINMODE}
CLEANFILES+=	pcs750.bin

all dependall: ${FILES}

pcs750.bin:	${.CURDIR}/pcs750.bin.uue
	${TOOL_UUDECODE} ${.ALLSRC}

.include <bsd.prog.mk>
