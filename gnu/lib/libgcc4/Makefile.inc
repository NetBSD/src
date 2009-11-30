#	$NetBSD: Makefile.inc,v 1.15 2009/11/30 11:55:12 uebayasi Exp $

.if ${MKGCC} != "no"
.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}.mk)
.include "${.CURDIR}/../arch/${MACHINE_ARCH}.mk"
.endif
.endif

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc4
GNUHOSTDIST=	${DIST}
GCCARCH=	${NETBSDSRCDIR}/gnu/usr.bin/gcc4/arch/${MACHINE_ARCH}
GCCARCHLIBGCC=	${NETBSDSRCDIR}/gnu/lib/libgcc4/arch/${MACHINE_ARCH}
GCCARCHXX=	${NETBSDSRCDIR}/gnu/lib/libstdc++-v3_4/arch/${MACHINE_ARCH}

GCPPFLAGS=	${G_LIBGCC2_CFLAGS} ${G_USE_COLLECT2} ${G_INCLUDES}
CPPFLAGS+=	-I${.CURDIR} -I${GCCARCHLIBGCC}
CPPFLAGS+=	-I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}
CPPFLAGS+=	-I${G_EXTRA_HEADERS:M/*:H:u}
CPPFLAGS+=	-I${DIST}/gcc/cp -I${DIST}/gcc/config -I${GCCARCHXX} -I.

# This is copied from gcc/mklibgcc.in:$lib2funcs
LIB2FUNCS_ALL=	_muldi3 _negdi2 _lshrdi3 _ashldi3 _ashrdi3 \
        _cmpdi2 _ucmpdi2 _floatdidf _floatdisf _fixunsdfsi _fixunssfsi \
        _fixunsdfdi _fixdfdi _fixunssfdi _fixsfdi _fixxfdi _fixunsxfdi \
        _floatdixf _fixunsxfsi _fixtfdi _fixunstfdi _floatditf _clear_cache \
        _enable_execute_stack _trampoline __main _absvsi2 _absvdi2 _addvsi3 \
        _addvdi3 _subvsi3 _subvdi3 _mulvsi3 _mulvdi3 _negvsi2 _negvdi2 _ctors \
        _ffssi2 _ffsdi2 _clz _clzsi2 _clzdi2 _ctzsi2 _ctzdi2 _popcount_tab \
        _popcountsi2 _popcountdi2 _paritysi2 _paritydi2 _powisf2 _powidf2 \
        _powixf2 _powitf2 \

LIB2FUNCS_ALL+=	\
	_mulsc3 _muldc3 _mulxc3 _multc3 _divsc3 _divdc3 _divxc3 _divtc3

LIB2FUNCS_SHORT:=${LIB2FUNCS_ALL}
LIB2_DIVMOD_FUNCS:=${G_LIB2_DIVMOD_FUNCS}
.for f in ${G_LIB1ASMFUNCS}; do
LIB2FUNCS_SHORT:=${LIB2FUNCS_SHORT:N${f}}
LIB2_DIVMOD_FUNCS:=${LIB2_DIVMOD_FUNCS:N${f}}
.endfor

LIB2FUNCS=	${LIB2FUNCS_SHORT:=.c}
LIB2FUNCS_ST=	${G_LIB2FUNCS_ST:=.c}
LIB2DIVMOD=	${LIB2_DIVMOD_FUNCS:=.c}
LIB2_EH=	${G_LIB2ADDEH:M*.c:T}
.if !empty(G_LIB1ASMFUNCS)
LIB1ASMFUNCS=	${G_LIB1ASMFUNCS:=.S}
.endif

DPSRCS+=	${.CURDIR}/../arch/${MACHINE_ARCH}.mk
CLEANFILES+=	${LIB2FUNCS} ${LIB2FUNCS_ST} ${LIB2DIVMOD} cs-tconfig.h
.if !empty(G_LIB1ASMFUNCS)
CLEANFILES+=	${LIB1ASMFUNCS}
.endif

.for file in ${G_LIB2ADD:M${GNUHOSTDIST}*}
BUILDSYMLINKS+=	${file} ${file:T:S/.asm$/.S/}
.endfor

# XXX
.if ${MACHINE_ARCH} == "m68000"
CPICFLAGS:=
COMPILE.S=	${CC} ${AFLAGS} ${CPPFLAGS} -c
_TRADITIONAL_CPP=
BUILDSYMLINKS+=	${DIST}/gcc/config/m68k/fpgnulib.c fpgnulib.c
BUILDSYMLINKS+=	${DIST}/gcc/config/m68k/fpgnulib.c xfgnulib.c
CPPFLAGS.xfgnulib.c+=-DEXTFLOAT=1
.endif

.if ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "sh3el" || \
    ${MACHINE_ARCH} == "sh3eb"
_TRADITIONAL_CPP=
COMPILE.S=	${CC} ${AFLAGS} ${CPPFLAGS} -c
.endif

.if ${MACHINE_ARCH} == "powerpc64"
COPTS+=	-mlong-double-128
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

${LIB2_EH:.c=.o}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} -fexceptions ${COPTS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC}

${LIB2_EH:.c=.so}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -fexceptions ${COPTS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC}

${G_LIB2_DIVMOD_FUNCS:=.o}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} -fexceptions -fnon-call-exceptions ${COPTS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC}

${G_LIB2_DIVMOD_FUNCS:=.so}:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -fexceptions -fnon-call-exceptions ${COPTS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC}

dp-bit.c: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define FLOAT\n#include <fp-bit.c>\n' >${.TARGET}

.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/cp/inc ${DIST}/gcc/config \
	${G_CONFIGDIR}

tconfig.h:
	${_MKTARGET_CREATE}
	TARGET_CPU_DEFAULT="" \
	HEADERS="$(G_xm_include_list)" DEFINES="USED_FOR_TARGET $(G_xm_defines)" \
	${HOST_SH} $(GNUHOSTDIST)/gcc/mkconfig.sh tconfig.h
DPSRCS+=	tconfig.h
CLEANFILES+=	tconfig.h

BUILDSYMLINKS+=	${G_UNWIND_H} unwind.h
DPSRCS+=	unwind.h

# these aren't necessary but are #include'd
FAKEHEADERS=options.h insn-flags.h insn-constants.h
${FAKEHEADERS}:
	${_MKTARGET_CREATE}
	touch ${.TARGET}
DPSRCS+=	${FAKEHEADERS}
CLEANFILES+=	${FAKEHEADERS}
