#	$NetBSD: Makefile,v 1.3 1997/12/21 15:48:30 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	usd/07.mail
SRCS=	mail0.nr mail1.nr mail2.nr mail3.nr mail4.nr mail5.nr mail6.nr \
	mail7.nr mail8.nr mail9.nr maila.nr
MACROS=	-me

paper.ps: ${SRCS}
	${SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TBL} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
