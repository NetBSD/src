#	$NetBSD: Makefile,v 1.14 1997/03/13 23:23:38 cgd Exp $

.include "../../Makefile.inc"

CFLAGS += -I${.CURDIR} -I.

MAN = mset.1 map3270.5

SRCS += astosc.c map3270.c mset.c

# this and the dependency hacks below to make 'depend' target
# work right...

DEPSRCS += astosc.OUT map3270.c mset.c

PROG = mset

MKASTOSC!=cd $(.CURDIR)/../tools/mkastosc; \
	printf "xxx:\n\techo \$${.OBJDIR}/mkastosc\n" | \
	${MAKE} -r -s -f - xxx | grep mkastosc

astosc.o: astosc.OUT
CLEANFILES += astosc.OUT astosc.out
astosc.OUT: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    ${.CURDIR}/../ctlr/${KBD} ${MKASTOSC}
	${MKASTOSC} \
		${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
		< ${.CURDIR}/../ctlr/${KBD} > ${.TARGET}
	/bin/rm -f astosc.out
	ln -s astosc.OUT astosc.out

# astosc.out

${MKASTOSC}:
	cd ${.CURDIR}/../tools/mkastosc; ${MAKE}


depend: .depend
.depend: ${DEPSRCS}
	${MKDEP} ${MKDEPFLAGS} ${CFLAGS:M-[ID]*} ${.ALLSRC:M*.c}

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii
