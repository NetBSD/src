#	$NetBSD: Makefile.inc,v 1.4 1995/09/23 03:42:17 gwr Exp $

.if defined(SA_PROG)

# Must have S=/usr/src/sys (or equivalent)
# But note: this is w.r.t. a subdirectory
S=	${.CURDIR}/../../../..

RELOC?= 240000
DEFS?= -DSTANDALONE
INCL?= -I${.CURDIR} -I${.CURDIR}/../libsa -I${S}/lib/libsa -I${S}
COPTS?= -msoft-float ${DEFS} ${INCL}

LIBSA?= ../libsa
SRTOBJ?= ${LIBSA}/SRT0.o ${LIBSA}/SRT1.o
LIBS?= ${LIBSA}/libsa.a
MDEC_DIR?=/usr/mdec

SRCS?=	${SA_PROG}.c
OBJS?=	${SRCS:S/.c/.o/g}

${SA_PROG}.bin : ${SA_PROG}
	cp ${SA_PROG} a.out ; strip a.out
	tail -c +33 < a.out > $@
	-rm -f a.out

${SA_PROG} : ${SRTOBJ} ${OBJS} ${LIBS}
	${LD} -N -T ${RELOC} -e start -o $@ ${SRTOBJ} ${OBJS} ${LIBS}
	@size $@

.if !target(clean)
clean:
	-rm -f a.out [Ee]rrs mklog core *.core
	-rm -f ${SA_PROG} ${SA_PROG}.bin ${OBJS} ${CLEANFILES}
.endif

.endif
