#	$NetBSD: Makefile.inc,v 1.9 1997/03/24 17:48:19 gwr Exp $

.if defined(SA_PROG)

# Allow "make CWD=." to show "normal" -I paths.
CWD=	${.CURDIR}

# Must have S=/usr/src/sys (or equivalent)
# But note: this is w.r.t. a subdirectory
S=	${CWD}/../../../..

RELOC?= 240000
DEFS?= -Dsun3 -DSTANDALONE
INCL?= -I${CWD} -I${CWD}/../libsa -I${S}/lib/libsa -I${S}
COPTS?= -msoft-float ${DEFS} ${INCL} ${DBG}

LIBSA?= ../libsa
SRTOBJ?= ${LIBSA}/SRT0.o ${LIBSA}/SRT1.o
SRTLIB?= ${LIBSA}/libsa.a

MDEC_DIR?=/usr/mdec

SRCS?=	${SA_PROG}.c
OBJS?=	${SRCS:S/.c/.o/g}

CLEANFILES+= ${SA_PROG} ${SA_PROG}.bin

# Make a copy of the executable with its exec header removed,
# and with its length padded to a multiple of 1k bytes.
# (The padding is for convenience when making tapes.)
${SA_PROG}.bin : ${SA_PROG}
	cp ${SA_PROG} a.out ; strip a.out
	dd if=a.out ibs=32 skip=1 of=$@ obs=1k conv=osync
	-rm -f a.out

${SA_PROG} : ${OBJS} ${DPADD}
	${LD} -N -Ttext ${RELOC} -e start -o $@ \
	  ${SRTOBJ} ${OBJS} ${LDADD} ${SRTLIB}
	@size $@

.if !target(clean)
clean:
	-rm -f a.out [Ee]rrs mklog core *.core
	-rm -f ${CLEANFILES} *.o
.endif

.if !target(install)
install: ${SA_PROG}.bin
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	  ${SA_PROG}.bin ${DESTDIR}${MDEC_DIR}/${SA_PROG}
.endif

.endif
