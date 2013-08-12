#	$NetBSD: Makefile.inc,v 1.19 2013/08/12 16:34:05 joerg Exp $

# Must have S=/usr/src/sys (or equivalent)
# But note: this is w.r.t. a subdirectory
S=	${.CURDIR}/../../../..

.if defined(SA_PROG)

.include <bsd.own.mk>

WARNS?=	2

LIBSA!=cd ${.CURDIR}/../libsa && ${PRINTOBJDIR}

MDEC_DIR?=/usr/mdec

RELOC?= 240000
DEFS?= -Dsun3 -D_STANDALONE -D__daddr_t=int32_t
INCL?= -I. -I${.CURDIR} -I${.CURDIR}/../libsa -I${S}/lib/libsa -I${S}

CPUFLAGS= -mc68000 -Wa,-mc68010
COPTS= -Os -fno-defer-pop -ffreestanding -fno-unwind-tables
CFLAGS= -msoft-float
CPPFLAGS= ${DEFS} ${DEBUG} ${INCL}

SRTOBJ= ${LIBSA}/SRT0.o ${LIBSA}/SRT1.o
SRTLIB= ${LIBSA}/lib/sa/libsa.a ${LIBSA}/lib/kern/libkern.a

CLEANFILES+= ${SA_PROG} ${SA_PROG}.bin

# Make a copy of the executable with its ELF headers removed.
${SA_PROG}.bin : ${SA_PROG}
	${OBJCOPY} --output-target=binary ${SA_PROG} $@

${SA_PROG} : ${OBJS} ${DPADD} ${SRTLIB}
	${LD} -N -Ttext ${RELOC} -e start ${SA_LDFLAGS} -o $@ \
	  ${SRTOBJ} ${OBJS} ${LDADD} ${SRTLIB}
	@${SIZE} $@

.if !target(proginstall)
realall: ${SA_PROG} ${SA_PROG}.bin
proginstall: ${SA_PROG}.bin
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
		${SA_PROG}.bin ${DESTDIR}${MDEC_DIR}/${SA_PROG}
.endif

.endif	# defined(SA_PROG)

.include <bsd.klinks.mk>

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
