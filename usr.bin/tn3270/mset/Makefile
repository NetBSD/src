#	$NetBSD: Makefile,v 1.18 1997/05/09 07:50:32 mycroft Exp $

.include "../../Makefile.inc"

CFLAGS += -I${.CURDIR} -I.

MAN = mset.1 map3270.5

SRCS += astosc.OUT astosc.c map3270.c mset.c

# this and the dependency hacks below to make 'depend' target
# work right...

PROG = mset

MKASTOSC!=cd $(.CURDIR)/../tools/mkastosc; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkastosc\n" | ${MAKE} -s -f-

CLEANFILES += astosc.OUT astosc.out
astosc.OUT: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    ${.CURDIR}/../ctlr/${KBD} ${MKASTOSC}
	${MKASTOSC} \
		${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
		< ${.CURDIR}/../ctlr/${KBD} > ${.TARGET}
	/bin/rm -f astosc.out
	ln -s astosc.OUT astosc.out

${MKASTOSC}:
	cd ${.CURDIR}/../tools/mkastosc; ${MAKE}

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii
