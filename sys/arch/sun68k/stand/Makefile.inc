#	$NetBSD: Makefile.inc,v 1.13.66.1 2009/01/19 13:16:57 skrll Exp $

# Must have S=/usr/src/sys (or equivalent)
# But note: this is w.r.t. a subdirectory
S=	${.CURDIR}/../../../..

.if defined(SA_PROG)

.include <bsd.own.mk>

LIBSA!=cd ${.CURDIR}/../libsa && ${PRINTOBJDIR}

MDEC_DIR?=/usr/mdec

RELOC?= 240000
DEFS?= -Dsun3 -D_STANDALONE -D__daddr_t=int32_t
INCL?= -I. -I${.CURDIR} -I${.CURDIR}/../libsa -I${S}/lib/libsa -I${S}

CPUFLAGS= -mc68000 -Wa,-mc68010
COPTS= -Os -fno-defer-pop -ffreestanding
CFLAGS= -msoft-float
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CPPFLAGS= ${DEFS} ${DEBUG} ${INCL}

SRTOBJ= ${LIBSA}/SRT0.o ${LIBSA}/SRT1.o
SRTLIB= ${LIBSA}/libsa.a

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

CLEANFILES+= machine m68k sun68k
.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN: machine m68k sun68k
.NOPATH: machine m68k sun68k
.endif

machine :
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

m68k :
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@

sun68k :
	-rm -f $@
	ln -s ${S}/arch/sun68k/include $@
