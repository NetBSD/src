#	$NetBSD: Makefile,v 1.3 1997/12/21 15:53:38 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	smm/07.lpd
SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t 7.t
MACROS=	-ms

paper.ps: ${SRCS}
	${SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TBL} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
