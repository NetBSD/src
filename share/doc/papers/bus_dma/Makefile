#	$NetBSD: Makefile,v 1.2 1998/07/15 03:11:34 thorpej Exp $

DIR=	papers/bus_dma
SRCS=	title.me 1.me 2.me 3.me 4.me 5.me about.me
MACROS=	-me
DPSRCS=	${SRCS} Makefile figure1.pic figure2.pic figure3.pic

paper.ps: ${DPSRCS}
	${SOELIM} ${SRCS} | ${PIC} | ${TBL} | ${ROFF} > ${.TARGET}

paper.dvi: ${DPSRCS}
	${SOELIM} ${SRCS} | ${PIC} | ${TBL} | ${ROFF} -Tdvi > ${.TARGET}

.include <bsd.doc.mk>
