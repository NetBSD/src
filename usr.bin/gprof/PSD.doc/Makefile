#	$NetBSD: Makefile,v 1.4 1997/12/21 15:46:33 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 8/14/93

DIR=	psd/18.gprof
SRCS=	header.me abstract.me intro.me profiling.me gathering.me \
	postp.me present.me refs.me
EXTRA=	postp1.pic postp2.pic postp3.pic pres1.pic pres2.pic
DPADD=	${EXTRA}
MACROS=	-me

paper.ps: ${SRCS} ${DPADD}
	${SOELIM} -I${.CURDIR} ${.ALLSRC:M*.me} | ${PIC} | ${TBL} | ${EQN} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
