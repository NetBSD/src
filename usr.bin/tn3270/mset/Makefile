# $Header: /cvsroot/src/usr.bin/tn3270/mset/Attic/Makefile,v 1.2 1993/04/27 06:47:37 cgd Exp $

.include <../../Makefile.inc>

CFLAGS += -I${.CURDIR} -I.

MAN1 = mset.0
MAN5 = map3270.0

SRCS += astosc.c map3270.c mset.c

# this and the dependency hacks below to make 'depend' target
# work right...

DEPSRCS += astosc.OUT

PROG = mset

astosc.o: astosc.OUT
CLEANFILES += astosc.OUT astosc.out
astosc.OUT: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    ${.CURDIR}/../ctlr/${KBD} ${.CURDIR}/../tools/mkastosc/obj/mkastosc
	${.CURDIR}/../tools/mkastosc/obj/mkastosc \
		${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
		< ${.CURDIR}/../ctlr/${KBD} > ${.TARGET}
	ln -s astosc.OUT astosc.out

# astosc.out

${.CURDIR}/../tools/mkastosc/obj/mkastosc:
        cd ${.CURDIR}/../tools/mkastosc; make


depend: .depend
.depend: ${SRCS} ${DEPSRCS}
	mkdep ${MKDEP} ${CFLAGS:M-[ID]*} ${.ALLSRC:M*.c}

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii
