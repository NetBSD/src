#	$NetBSD: Makefile.inc,v 1.1.2.2 2002/06/23 17:42:47 jdolecek Exp $

# Must have S=/usr/src/sys (or equivalent)
# But note: this is w.r.t. a subdirectory
S=	${.CURDIR}/../../../..

.if defined(SA_PROG)

.include <bsd.own.mk>

LIBSA!=cd ${.CURDIR}/../libsa && ${PRINTOBJDIR}

MDEC_DIR?=/usr/mdec

RELOC?= 240000
DEFS?= -Dsun3 -D_STANDALONE
INCL?= -I. -I${.CURDIR} -I${.CURDIR}/../libsa -I${S}/lib/libsa -I${S}

CFLAGS= -Os -msoft-float -fno-defer-pop
CPPFLAGS= ${DEFS} ${DBG} ${INCL}

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
.BEGIN: machine m68k sun68k
.NOPATH: machine m68k sun68k
realall realdepend: machine m68k sun68k

machine :
	-rm -f $@
	ln -s ${S}/arch/sun68k/include $@

m68k :
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@

sun68k :
	-rm -f $@
	ln -s ${S}/arch/sun68k $@
