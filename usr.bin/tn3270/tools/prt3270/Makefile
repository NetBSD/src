#	$NetBSD: Makefile,v 1.7 1999/07/14 21:36:39 wrstuden Exp $

HOSTPROG=	prt3270
SRCS=	prt3270.c asc_ebc.c ebc_disp.c astosc.c
DPSRCS=	kbd.out astosc.out
HOST_CPPFLAGS+=-I.

MKHITS!=cd $(.CURDIR)/../mkhits; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkhits\n" | ${MAKE} -s -f-
${MKHITS}:
	cd ${.CURDIR}/../mkhits; ${MAKE}

kbd.out: ${.CURDIR}/../../ctlr/hostctlr.h ${.CURDIR}/../../ctlr/${KBD} ${MKHITS}
	${HOST_CC} ${HOST_CPPFLAGS} -E ${.CURDIR}/../../ctlr/function.c > TMPfunc.out
	${MKHITS} \
	    ${.CURDIR}/../../ctlr/hostctlr.h TMPfunc.out \
	    < ${.CURDIR}/../../ctlr/${KBD} > kbd.tmp
	rm -f TMPfunc.out
	mv -f kbd.tmp ${.TARGET}

MKASTOSC!=cd $(.CURDIR)/../mkastosc; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkastosc\n" | ${MAKE} -s -f-
${MKASTOSC}:
	cd ${.CURDIR}/../mkastosc; ${MAKE}

astosc.out: ${.CURDIR}/../../ctlr/hostctlr.h ${.CURDIR}/../../ctlr/function.h \
	    ${.CURDIR}/../../ctlr/${KBD} ${MKASTOSC}
	${MKASTOSC} \
	    ${.CURDIR}/../../ctlr/hostctlr.h ${.CURDIR}/../../ctlr/function.h \
	    < ${.CURDIR}/../../ctlr/${KBD} > tmp
	mv -f tmp ${.TARGET}

.include <bsd.hostprog.mk>

.PATH: ${.CURDIR}/../../api ${.CURDIR}/../../ascii

astosc.o:	astosc.out
prt3270.o:	kbd.out
