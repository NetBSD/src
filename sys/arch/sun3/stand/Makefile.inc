#	$NetBSD: Makefile.inc,v 1.12.14.3 2001/03/12 13:29:36 bouyer Exp $

.if defined(SA_PROG)

# Must have S=/usr/src/sys (or equivalent)
# But note: this is w.r.t. a subdirectory
S=	${.CURDIR}/../../../..

LIBSA!=cd ${.CURDIR}/../libsa; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

MDEC_DIR?=/usr/mdec

RELOC?= 240000
DEFS?= -Dsun3 -D_STANDALONE
INCL?= -I. -I${.CURDIR} -I${.CURDIR}/../libsa -I${S}/lib/libsa -I${S}

CFLAGS= -O2 -msoft-float -fno-defer-pop
CPPFLAGS= ${DEFS} ${DBG} ${INCL}

SRTOBJ= ${LIBSA}/SRT0.o ${LIBSA}/SRT1.o
SRTLIB= ${LIBSA}/libsa.a

CLEANFILES+= ${SA_PROG} ${SA_PROG}.bin machine m68k

# Make a copy of the executable with its ELF headers removed.
${SA_PROG}.bin : ${SA_PROG}
	${OBJCOPY} --output-target=binary ${SA_PROG} $@

${SA_PROG} : ${OBJS} ${DPADD} ${SRTLIB}
	${LD} -N -Ttext ${RELOC} -e start -o $@ \
	  ${SRTOBJ} ${OBJS} ${LDADD} ${SRTLIB}
	@size $@

${OBJS} : machine m68k

.if !target(proginstall)
realall: ${SA_PROG} ${SA_PROG}.bin
proginstall: ${SA_PROG}.bin
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	  ${SA_PROG}.bin ${DESTDIR}${MDEC_DIR}/${SA_PROG}
.endif

.endif	# defined(SA_PROG)

machine :
	-rm -f $@
	ln -s ${S}/arch/sun3/include $@

m68k :
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@
