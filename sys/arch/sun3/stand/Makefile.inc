#	$NetBSD: Makefile.inc,v 1.1.1.1 1995/02/14 22:56:36 gwr Exp $

.if defined(SA_PROG)

# Must have S=/usr/src/sys (or equivalent)
# But note: this is w.r.t. a subdirectory
S=	../../../..

RELOC?= 240000
DEFS?= -DSTANDALONE
INCL?= -I. -I../libsa -I${S}/lib/libsa -I${S}
COPTS?= -msoft-float ${DEFS} ${INCL}

LIBSA?= ../libsa
SRTOBJ?= ${LIBSA}/SRT0.o ${LIBSA}/SRT1.o
LIBS?= ${LIBSA}/libsa.a

SRCS?=	${SA_PROG}.c
OBJS?=	${SRTOBJ} ${SRCS:S/.c/.o/g}

all:	${SA_PROG}

${SA_PROG} : ${OBJS} ${LIBS}
	${LD} -N -T ${RELOC} -e start -o $@ ${OBJS} ${LIBS}
	@size $@

.endif
