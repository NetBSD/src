#	$NetBSD: Makefile,v 1.18 2003/12/11 22:37:27 matt Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		gcc

.cc: # disable .cc->NULL transform

.if exists(${.CURDIR}/${MACHINE_ARCH}.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/${MACHINE_ARCH}.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc
GNUHOSTDIST=	${DIST}
GCCARCH=	${NETBSDSRCDIR}/gnu/usr.bin/gcc3/arch/${MACHINE_ARCH}
GCCARCHXX=	${NETBSDSRCDIR}/gnu/lib/libstdc++-v3/arch/${MACHINE_ARCH}

GCPPFLAGS=	${G_LIBGCC2_CFLAGS} ${G_MAYBE_USE_COLLECT2} ${G_INCLUDES}
CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}
CPPFLAGS+=	-I${DIST}/gcc/cp -I${GCCARCHXX} -I.

LIB2FUNCS=	${G_LIB2FUNCS_1:=.c} ${G_LIB2FUNCS_2:=.c} ${G_LIB2FUNCS_ST:=.c}
LIB2DIVMOD=	${G_LIB2_DIVMOD_FUNCS:=.c}
LIB2_EH=	${G_LIB2ADDEH:M*.c:T}
LIB1ASMFUNCS=	${G_LIB1ASMFUNCS:=.S}

SRCS+=		${LIB2FUNCS} ${LIB2DIVMOD} ${LIB2_EH} \
		${G_LIB2ADD:T:S/.asm/.S/} ${LIB1ASMFUNCS} \
		${G_LIB2FUNCS_EXTRA}

DPSRCS+=	${.CURDIR}/${MACHINE_ARCH}.mk tconfig.h
CLEANFILES+=	${LIB2FUNCS} ${LIB2DIVMOD} cs-tconfig.h tconfig.h

# XXX
.if ${MACHINE_ARCH} == "m68000"
CPICFLAGS:=
COMPILE.S=	${CC} ${AFLAGS} ${CPPFLAGS} -c
_TRADITIONAL_CPP=
fpgnulib.c: ${DIST}/gcc/config/m68k/fpgnulib.c
	cp ${DIST}/gcc/config/m68k/fpgnulib.c fpgnulib.c
xfgnulib.c: ${DIST}/gcc/config/m68k/fpgnulib.c
	echo '#define EXTFLOAT' > xfgnulib.c
	cat ${DIST}/gcc/config/m68k/fpgnulib.c >> xfgnulib.c
.endif
.if ${MACHINE_ARCH} == "powerpc"
_TRADITIONAL_CPP=
COMPILE.S=	${CC} ${AFLAGS} ${CPPFLAGS} -c
.endif

${LIB2FUNCS}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

${LIB2DIVMOD}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

.if !empty(LIB1ASMFUNCS)
${LIB1ASMFUNCS}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <${G_LIB1ASMSRC}>\n' >${.TARGET}
.endif

${G_LIB2ADD:T:S/.asm/.S/}: ${.CURDIR}/Makefile ${G_LIB2ADD}
	@echo copying ${G_LIB2ADD}
	for i in ${G_LIB2ADD}; do \
		j=$${i##*/}; \
		j=`echo $$j | sed 's/\.asm$$/\.S/'`; \
		cp $$i $$j; \
	done
	@echo copying ${G_LIB2ADD} complete

${LIB2_EH:.c=.o}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -fexceptions -o ${.TARGET} ${.IMPSRC}

${G_LIB2_DIVMOD_FUNCS:=.o}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -fexceptions -fnon-call-exceptions -o ${.TARGET} ${.IMPSRC}

dp-bit.c: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define FLOAT\n#include <fp-bit.c>\n' >${.TARGET}

.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/cp/inc ${DIST}/gcc/config \
	${G_CONFIGDIR}

.include <bsd.lib.mk>

tconfig.h:
	${_MKTARGET_CREATE}
	TM_DEFINES="$(G_tm_defines)" \
	HEADERS="$(G_xm_file)" XM_DEFINES="$(G_xm_defines)" \
	TARGET_CPU_DEFAULT="" \
	${HOST_SH} $(GNUHOSTDIST)/gcc/mkconfig.sh tconfig.h

.else
.include <bsd.prog.mk> # do nothing
.endif
