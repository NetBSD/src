#	$NetBSD: Makefile,v 1.1 1998/07/15 00:34:54 thorpej Exp $

SRCS=	title.me 1.me 2.me 3.me 4.me 5.me about.me
MACROS=	-me
DPSRCS=	${SRCS} Makefile figure1.pic figure2.pic figure3.pic

paper.ps: ${DPSRCS}
	${SOELIM} ${SRCS} | ${PIC} | ${TBL} | ${ROFF} > ${.TARGET}

paper.dvi: ${DPSRCS}
	${SOELIM} ${SRCS} | ${PIC} | ${TBL} | ${ROFF} -Tdvi > ${.TARGET}

.include <bsd.doc.mk>
