#	$NetBSD: Makefile.inc,v 1.6.2.1 2004/03/29 19:26:42 jdc Exp $

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/arch/${MACHINE_ARCH}.mk"
.endif

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc
GNUHOSTDIST=	${DIST}
GCCARCH=	${NETBSDSRCDIR}/gnu/usr.bin/gcc3/arch/${MACHINE_ARCH}
GCCARCHXX=	${NETBSDSRCDIR}/gnu/lib/libstdc++-v3/arch/${MACHINE_ARCH}

GCPPFLAGS=	${G_LIBGCC2_CFLAGS} ${G_USE_COLLECT2} ${G_INCLUDES}
CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}
CPPFLAGS+=	-I${G_EXTRA_HEADERS:H:u}
CPPFLAGS+=	-I${DIST}/gcc/cp -I${GCCARCHXX} -I.

LIB2FUNCS=	${G_LIB2FUNCS_1:=.c} ${G_LIB2FUNCS_2:=.c}
LIB2FUNCS_ST=	${G_LIB2FUNCS_ST:=.c}
LIB2DIVMOD=	${G_LIB2_DIVMOD_FUNCS:=.c}
LIB2_EH=	${G_LIB2ADDEH:M*.c:T}
.if !empty(G_LIB1ASMFUNCS)
LIB1ASMFUNCS=	${G_LIB1ASMFUNCS:=.S}
.endif

DPSRCS+=	${.CURDIR}/../arch/${MACHINE_ARCH}.mk tconfig.h
CLEANFILES+=	${LIB2FUNCS} ${LIB2FUNCS_ST} ${LIB2DIVMOD} cs-tconfig.h \
		tconfig.h
.if !empty(G_LIB1ASMFUNCS)
CLEANFILES+=	${LIB1ASMFUNCS}
.endif

# XXX
.if ${MACHINE_ARCH} == "m68000"
CPICFLAGS:=
COMPILE.S=	${CC} ${AFLAGS} ${CPPFLAGS} -c
_TRADITIONAL_CPP=
fpgnulib.c:	${DIST}/gcc/config/m68k/fpgnulib.c
	cp ${DIST}/gcc/config/m68k/fpgnulib.c fpgnulib.c
xfgnulib.c:	${DIST}/gcc/config/m68k/fpgnulib.c
	echo '#define EXTFLOAT' > xfgnulib.c
	cat ${DIST}/gcc/config/m68k/fpgnulib.c >> xfgnulib.c
CLEANFILES+=	fpgnulib.c xfgnulib.c
.endif
.if ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "sh3el" || \
    ${MACHINE_ARCH} == "sh3eb" || \
    ${MACHINE_ARCH} == "sh5el" || \
    ${MACHINE_ARCH} == "sh5eb"
_TRADITIONAL_CPP=
COMPILE.S=	${CC} ${AFLAGS} ${CPPFLAGS} -c
.endif

${LIB2FUNCS} ${LIB2FUNCS_ST}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

${LIB2DIVMOD}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

.if !empty(G_LIB1ASMFUNCS)
${LIB1ASMFUNCS}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <${G_LIB1ASMSRC}>\n' >${.TARGET}
.endif

${G_LIB2ADD:M${GNUHOSTDIST}/*:T:S/.asm/.S/}: ${.CURDIR}/Makefile ${G_LIB2ADD}
	@echo copying ${G_LIB2ADD}
	for i in ${G_LIB2ADD}; do \
		j=$${i##*/}; \
		j=`echo $$j | sed 's/\.asm$$/\.S/'`; \
		cp $$i $$j; \
	done
	@echo copying ${G_LIB2ADD} complete

${LIB2_EH:.c=.o}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} -fexceptions -o ${.TARGET} ${.IMPSRC}

${LIB2_EH:.c=.so}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -fexceptions -o ${.TARGET} ${.IMPSRC}

${G_LIB2_DIVMOD_FUNCS:=.o}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} -fexceptions -fnon-call-exceptions -o ${.TARGET} ${.IMPSRC}

${G_LIB2_DIVMOD_FUNCS:=.so}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -fexceptions -fnon-call-exceptions -o ${.TARGET} ${.IMPSRC}

dp-bit.c: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define FLOAT\n#include <fp-bit.c>\n' >${.TARGET}

.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/cp/inc ${DIST}/gcc/config \
	${G_CONFIGDIR}

tconfig.h:
	${_MKTARGET_CREATE}
	TM_DEFINES="$(G_tm_defines)" \
	HEADERS="$(G_xm_file)" XM_DEFINES="$(G_xm_defines)" \
	TARGET_CPU_DEFAULT="" \
	${HOST_SH} $(GNUHOSTDIST)/gcc/mkconfig.sh tconfig.h
