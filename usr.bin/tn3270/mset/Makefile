#	$NetBSD: Makefile,v 1.22 1999/03/06 16:21:58 itohy Exp $

PROG=	mset
MAN=	mset.1 map3270.5
SRCS=	astosc.c map3270.c mset.c
DPSRCS=	astosc.out

MKASTOSC!=cd $(.CURDIR)/../tools/mkastosc; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkastosc\n" | ${MAKE} -s -f-
${MKASTOSC}:
	cd ${.CURDIR}/../tools/mkastosc; ${MAKE}

astosc.out: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    ${.CURDIR}/../ctlr/${KBD} ${MKASTOSC}
	${MKASTOSC} \
	    ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    < ${.CURDIR}/../ctlr/${KBD} > tmp
	mv -f tmp ${.TARGET}

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii

astosc.o:	astosc.out
