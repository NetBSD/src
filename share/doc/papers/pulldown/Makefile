#	$Id: Makefile,v 1.1.2.1 2002/06/05 05:54:54 lukem Exp $

DIR=	papers/pulldown
SRCS=	0.t 1.t 2.t 4.t 8.t 9.t
MACROS=	-ms
DPSRCS=	${SRCS} refs.r Makefile

paper.ps: ${DPSRCS}
	${SOELIM} -I${.CURDIR} ${SRCS} | \
	    ${REFER} -P -S -e -p ${.CURDIR}/refs.r | \
	    ${PIC} | ${TBL} | ${EQN} |  ${ROFF} ${MACROS} > ${.TARGET}

paper.dvi: ${DPSRCS}
	${SOELIM} -I${.CURDIR} ${SRCS} | \
	    ${REFER} -P -S -e -p ${.CURDIR}/refs.r | \
	    ${PIC} | ${TBL} | ${ROFF} ${MACROS} -Tdvi > ${.TARGET}

paper.txt: ${DPSRCS}
	${SOELIM} -I${.CURDIR} ${SRCS} | \
	    ${REFER} -P -S -e -p ${.CURDIR}/refs.r | \
	    ${PIC} | ${TBL} | ${EQN} -Tascii |  nroff -ms > ${.TARGET}

.include <bsd.doc.mk>
