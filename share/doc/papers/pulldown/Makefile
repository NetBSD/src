#	$Id: Makefile,v 1.1 2001/07/04 05:29:25 itojun Exp $

DIR=	papers/pulldown
SRCS=	0.t 1.t 2.t 4.t 8.t 9.t
MACROS=	-ms
DPSRCS=	${SRCS} refs.r Makefile

paper.ps: ${DPSRCS}
	${SOELIM} -I${.CURDIR} ${SRCS} | \
	    ${REFER} -P -S -e -p ${.CURDIR}/refs.r | \
	    ${PIC} | ${TBL} | ${EQN} |  ${ROFF} > ${.TARGET}

paper.dvi: ${DPSRCS}
	${SOELIM} -I${.CURDIR} ${SRCS} | \
	    ${REFER} -P -S -e -p ${.CURDIR}/refs.r | \
	    ${PIC} | ${TBL} | ${ROFF} -Tdvi > ${.TARGET}

paper.txt: ${DPSRCS}
	${SOELIM} -I${.CURDIR} ${SRCS} | \
	    ${REFER} -P -S -e -p ${.CURDIR}/refs.r | \
	    ${PIC} | ${TBL} | ${EQN} -Tascii |  nroff -ms > ${.TARGET}

.include <bsd.doc.mk>
