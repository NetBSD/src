#	$NetBSD: Makefile,v 1.2.12.1 2002/06/05 05:54:45 lukem Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 4/20/94

DIR=	papers/nqnfs
SRCS=	nqnfs.me
MACROS=	-me

paper.ps: ${SRCS}
	${PIC} ${SRCS} | ${TBL} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
