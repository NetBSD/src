#	$NetBSD: Makefile,v 1.4 2003/01/05 12:30:31 pooka Exp $
#
#	@(#)Makefile	8.2 (Berkeley) 5/11/94

DIR=	smm/12.timed
SRCS=	timed.ms
MACROS=	-ms

paper.ps: ${SRCS}
	${SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TBL} | ${ROFF} ${MACROS}	\
	    > ${.TARGET}

.include <bsd.doc.mk>
