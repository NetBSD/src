#	$NetBSD: Makefile,v 1.4 1998/07/15 11:56:34 drochner Exp $

DIR=	papers/bus_dma
SRCS=	title.me 1.me 2.me 3.me 4.me 5.me about.me
EXTRA=	figure1.pic figure2.pic figure3.pic
MACROS=	-me
DPSRCS=	${SRCS} ${EXTRA} Makefile

paper.ps: ${DPSRCS}
	${SOELIM} -I${.CURDIR} ${SRCS} | ${PIC} | ${TBL} | ${ROFF} > ${.TARGET}

paper.dvi: ${DPSRCS}
	${SOELIM} -I${.CURDIR} ${SRCS} | ${PIC} | ${TBL} | ${ROFF} -Tdvi \
		> ${.TARGET}

.include <bsd.doc.mk>
