#	$NetBSD: Makefile,v 1.2 1998/01/09 06:41:41 perry Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 4/20/94

DIR=	papers/nqnfs
SRCS=	nqnfs.me
MACROS=	-me

paper.ps: ${SRCS}
	${PIC} ${SRCS} | ${TBL} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
