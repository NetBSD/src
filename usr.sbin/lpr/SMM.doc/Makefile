#	$NetBSD: Makefile,v 1.2 1995/11/15 22:54:37 pk Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	smm/07.lpd
SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t 7.t
MACROS=	-ms

paper.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
