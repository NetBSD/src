#	$NetBSD: Makefile,v 1.1 1998/03/04 13:16:14 christos Exp $

PROG=	prt3270
SRCS=	prt3270.c asc_ebc.c ebc_disp.c astosc.c
DPSRCS=	kbd.out astosc.out
CPPFLAGS+=-I.

MKHITS!=cd $(.CURDIR)/../mkhits; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkhits\n" | ${MAKE} -s -f-
${HITS}:
	cd ${.CURDIR}/../mkhits; ${MAKE}

kbd.out: ${.CURDIR}/../../ctlr/hostctlr.h ${.CURDIR}/../../ctlr/${KBD} ${MKHITS}
	${CC} ${CPPFLAGS} -E ${.CURDIR}/../../ctlr/function.c > TMPfunc.out
	${MKHITS} \
	    ${.CURDIR}/../../ctlr/hostctlr.h TMPfunc.out \
	    < ${.CURDIR}/../../ctlr/${KBD} > kbd.tmp
	rm -f TMPFunc.out
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

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../../api ${.CURDIR}/../../ascii
