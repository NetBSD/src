#	$NetBSD: Makefile.inc,v 1.1 1997/05/19 21:19:20 is Exp $

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
	@cd ${AOUT2BBDIR} && make depend && make

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
	@cd ${TXLTDIR} && make depend && make
