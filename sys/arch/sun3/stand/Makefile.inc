#	$NetBSD: Makefile.inc,v 1.13 2000/07/16 21:56:12 jdolecek Exp $

.if defined(SA_PROG)

# Must have S=/usr/src/sys (or equivalent)
# But note: this is w.r.t. a subdirectory
S=	${.CURDIR}/../../../..

LIBSA!=cd ${.CURDIR}/../libsa; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

MDEC_DIR?=/usr/mdec

SRCS?=	${SA_PROG}.c
OBJS?=	${SRCS:.c=.o}

RELOC?= 240000
DEFS?= -Dsun3 -D_STANDALONE
INCL?= -I. -I${.CURDIR} -I${.CURDIR}/../libsa -I${S}/lib/libsa -I${S}

CFLAGS= -O2 -msoft-float -fno-defer-pop
CPPFLAGS= ${DEFS} ${DBG} ${INCL}

SRTOBJ= ${LIBSA}/SRT0.o ${LIBSA}/SRT1.o
SRTLIB= ${LIBSA}/libsa.a

CLEANFILES+= ${SA_PROG} ${SA_PROG}.bin machine m68k

# Make a copy of the executable with its exec header removed,
# and with its length padded to a multiple of 1k bytes.
# (The padding is for convenience when making tapes.)
${SA_PROG}.bin : ${SA_PROG}
	cp ${SA_PROG} ${SA_PROG}.tmp
	strip ${SA_PROG}.tmp
	dd if=${SA_PROG}.tmp ibs=32 skip=1 obs=1k | \
	  dd of=$@ bs=1k conv=sync
	-rm -f ${SA_PROG}.tmp

${SA_PROG} : ${OBJS} ${DPADD} ${SRTLIB}
	${LD} -N -Ttext ${RELOC} -e start -o $@ \
	  ${SRTOBJ} ${OBJS} ${LDADD} ${SRTLIB}
	@size $@

${OBJS} : machine m68k

.if !target(cleanprog)
cleanprog:
	-rm -f ${SA_PROG}.tmp [Ee]rrs mklog core *.core
	-rm -f ${CLEANFILES} *.o
.endif

.if !target(proginstall)
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

