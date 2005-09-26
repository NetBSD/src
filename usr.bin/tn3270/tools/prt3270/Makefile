#	$NetBSD: Makefile,v 1.9.6.1 2005/09/26 20:29:05 tron Exp $

NOMAN=		# defined

.include <bsd.own.mk>

HOSTPROG=	prt3270
SRCS=	prt3270.c asc_ebc.c ebc_disp.c astosc.c
DPSRCS=	kbd.out astosc.out
HOST_CPPFLAGS+=-I.

MKHITSDIR!=cd $(.CURDIR)/../mkhits && ${PRINTOBJDIR}
MKHITS=${MKHITSDIR}/mkhits

${MKHITS}:
	cd ${.CURDIR}/../mkhits; ${MAKE}

kbd.out: ${.CURDIR}/../../ctlr/hostctlr.h ${.CURDIR}/../../ctlr/${KBD} ${MKHITS}
	${HOST_CC} ${HOST_CPPFLAGS} -E ${.CURDIR}/../../ctlr/function.c > TMPfunc.out
	${MKHITS} \
	    ${.CURDIR}/../../ctlr/hostctlr.h TMPfunc.out \
	    < ${.CURDIR}/../../ctlr/${KBD} > kbd.tmp
	rm -f TMPfunc.out
	mv -f kbd.tmp ${.TARGET}
CLEANFILES+=	TMPfunc.out kbd.tmp kbd.out

MKASTOSCDIR!=cd ${.CURDIR}/../mkastosc && ${PRINTOBJDIR}
MKASTOSC= ${MKASTOSCDIR}/mkastosc

${MKASTOSC}:
	cd ${.CURDIR}/../mkastosc; ${MAKE}

astosc.out: ${.CURDIR}/../../ctlr/hostctlr.h ${.CURDIR}/../../ctlr/function.h \
	    ${.CURDIR}/../../ctlr/${KBD} ${MKASTOSC}
	${MKASTOSC} \
	    ${.CURDIR}/../../ctlr/hostctlr.h ${.CURDIR}/../../ctlr/function.h \
	    < ${.CURDIR}/../../ctlr/${KBD} > astosc.tmp
	mv -f astosc.tmp ${.TARGET}
CLEANFILES+=	astosc.tmp astosc.out

.include <bsd.hostprog.mk>

.PATH: ${.CURDIR}/../../api ${.CURDIR}/../../ascii

astosc.o:	astosc.out
prt3270.o:	kbd.out
