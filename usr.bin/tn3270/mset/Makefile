#	$Id: Makefile,v 1.7 1993/12/21 22:16:58 davidb Exp $

.include <../../Makefile.inc>

CFLAGS += -I${.CURDIR} -I.

MAN1 = mset.0
MAN5 = map3270.0

SRCS += astosc.c map3270.c mset.c

# this and the dependency hacks below to make 'depend' target
# work right...

DEPSRCS += astosc.OUT map3270.c mset.c

PROG = mset

.if exists(${.CURDIR}/../tools/mkastosc/obj.${MACHINE})
MKASTOSC=	$(.CURDIR)/../tools/mkastosc/obj.${MACHINE}/mkastosc
.elif exists(${.CURDIR}/../tools/mkastosc/obj)
MKASTOSC=	$(.CURDIR)/../tools/mkastosc/obj/mkastosc
.else
MKASTOSC=	$(.CURDIR)/../tools/mkastosc/mkastosc
.endif

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
	cd ${.CURDIR}/../tools/mkastosc; make


depend: .depend
.depend: ${DEPSRCS}
	mkdep ${MKDEP} ${CFLAGS:M-[ID]*} ${.ALLSRC:M*.c}

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii
