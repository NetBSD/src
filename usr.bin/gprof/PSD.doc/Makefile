#	$NetBSD: Makefile,v 1.5 2003/01/05 12:24:22 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 8/14/93

DIR=	psd/18.gprof
SRCS=	header.me abstract.me intro.me profiling.me gathering.me \
	postp.me present.me refs.me
EXTRA=	postp1.pic postp2.pic postp3.pic pres1.pic pres2.pic
DPADD=	${EXTRA}
MACROS=	-me

paper.ps: ${SRCS} ${DPADD}
	${SOELIM} -I${.CURDIR} ${.ALLSRC:M*.me} | ${PIC} | ${TBL} |	\
	    ${EQN} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
