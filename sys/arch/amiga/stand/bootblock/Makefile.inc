#	$NetBSD: Makefile.inc,v 1.2 2000/08/30 23:51:54 jhawk Exp $

BINDIR=		/usr/mdec

### Figure out what to use for aout2bb
AOUT2BBDIR=	${.CURDIR}/../aout2bb

.if exists(${AOUT2BBDIR}/obj.${MACHINE})
AOUT2BBOBJDIR=	${AOUT2BBDIR}/obj.${MACHINE}
.elif exists(${AOUT2BBDIR}/obj)
AOUT2BBOBJDIR=	${AOUT2BBDIR}/obj
.else
AOUT2BBOBJDIR=	${AOUT2BBDIR}
.endif
AOUT2BB=	${AOUT2BBOBJDIR}/aout2bb

${AOUT2BB}:
	@cd ${AOUT2BBDIR} && ${MAKE} depend && ${MAKE}

### Figure out what to use for txlt
TXLTDIR=	${.CURDIR}/../txlt

.if exists(${TXLTDIR}/obj.${MACHINE})
TXLTOBJDIR=	${TXLTDIR}/obj.${MACHINE}
.elif exists(${TXLTDIR}/obj)
TXLTOBJDIR=	${TXLTDIR}/obj
.else
TXLTOBJDIR=	${TXLTDIR}
.endif
TXLT=		${TXLTOBJDIR}/txlt

${TXLT}:
	@cd ${TXLTDIR} && ${MAKE} depend && ${MAKE}
