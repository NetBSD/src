#	$NetBSD: Makefile,v 1.25 2003/10/19 04:30:30 lukem Exp $

.include <bsd.own.mk>

PROG=	mset
MAN=	mset.1 map3270.5
SRCS=	astosc.c map3270.c mset.c
DPSRCS=	astosc.out

MKASTOSCDIR!=cd $(.CURDIR)/../tools/mkastosc && ${PRINTOBJDIR}
MKASTOSC=${MKASTOSCDIR}/mkastosc

${MKASTOSC}:
	@cd ${.CURDIR}/../tools/mkastosc && ${MAKE}

CLEANFILES+= astosc.out
astosc.out: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    ${.CURDIR}/../ctlr/${KBD} ${MKASTOSC}
	${_MKMSGCREATE}
	${_MKCMD}\
	${MKASTOSC} \
	    ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    < ${.CURDIR}/../ctlr/${KBD} > tmp
	${_MKCMD}\
	mv -f tmp ${.TARGET}

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii

astosc.o:	astosc.out
