#	$NetBSD: Makefile,v 1.3 1998/01/09 08:11:52 perry Exp $
#
#	@(#)Makefile	8.2 (Berkeley) 5/11/94

DIR=	smm/12.timed
SRCS=	timed.ms
MACROS=	-ms

paper.ps: ${SRCS}
	${SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TBL} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
