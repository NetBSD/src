#	$NetBSD: Makefile.inc,v 1.52 2022/07/22 21:59:11 mrg Exp $

LIBGCC_MACHINE_ARCH?=${MACHINE_ARCH:S/earmv5/earm/}
GCC_MACHINE_SUBDIR=${MACHINE_CPU:C/powerpc.*/rs6000/:C/x86_64/i386/}

.if ${MKGCC} != "no"
.if exists(${.CURDIR}/../arch/${LIBGCC_MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/../arch/${LIBGCC_MACHINE_ARCH}/defs.mk"
.endif
.endif

UNSUPPORTED_COMPILER.clang=	# defined

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}
GCCARCH=	${GCC_SUBDIR}/usr.bin/gcc/arch/${LIBGCC_MACHINE_ARCH}
GCCARCHLIBGCC=	${GCC_SUBDIR}/lib/libgcc/arch/${LIBGCC_MACHINE_ARCH}
GCCARCHXX=	${GCC_SUBDIR}/lib/libstdc++-v3/arch/${LIBGCC_MACHINE_ARCH}

GCPPFLAGS=	${G_LIBGCC2_CFLAGS} ${G_USE_COLLECT2} ${G_INCLUDES}
CPPFLAGS+=	-I${.CURDIR} -I${GCCARCHLIBGCC}
CPPFLAGS+=	-I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}
CPPFLAGS+=	${GCPPFLAGS:M-m*} ${GCPPFLAGS:M-f*}
CPPFLAGS+=	${G_EXTRA_HEADERS:M/*:H:u:S/^/-I/g}
CPPFLAGS+=	-I${DIST}/libgcc -I${DIST}/libgcc/config
CPPFLAGS+=	-I${DIST}/libgcc/config/${GCC_MACHINE_SUBDIR}
CPPFLAGS+=	-I${DIST}/gcc -I${DIST}/include -I.
CPPFLAGS+=	${G_INTERNAL_CFLAGS:M-DHAVE_CC_TLS}

.if ${LIBGCC_MACHINE_ARCH} == "powerpc" || \
    ${LIBGCC_MACHINE_ARCH} == "sh3el" || \
    ${LIBGCC_MACHINE_ARCH} == "sh3eb" || \
    ${LIBGCC_MACHINE_ARCH} == "m68000" || \
    ${LIBGCC_MACHINE_ARCH} == "aarch64" || \
    ${LIBGCC_MACHINE_ARCH} == "aarch64eb"
_TRADITIONAL_CPP=
COMPILE.S=	${CC} ${AFLAGS} ${CPPFLAGS} ${CPPFLAGS.${.IMPSRC:T}} -c 
.endif

.if ${LIBGCC_MACHINE_ARCH} == "powerpc"
CPPFLAGS+=	-I${DIST}/gcc/config/rs6000
.endif

#
# Old style libgcc build procedure.
#

# This is copied from dist/libgcc/Makefile.in:$lib2funcs
LIB2FUNCS_ALL= \
	_muldi3 _negdi2 _lshrdi3 _ashldi3 _ashrdi3 _cmpdi2 _ucmpdi2    \
	_clear_cache _trampoline __main \
	_absvsi2 _absvdi2 _addvsi3 _addvdi3 \
	_subvsi3 _subvdi3 _mulvsi3 _mulvdi3 \
	_negvsi2 _negvdi2 \
	_ctors \
	_ffssi2 _ffsdi2 _clz _clzsi2 _clzdi2  _ctzsi2 _ctzdi2 \
	_popcount_tab _popcountsi2 _popcountdi2 _paritysi2 _paritydi2 \
	_powisf2 _powidf2 _powixf2 _powitf2 \
	_mulsc3 _muldc3 _mulxc3 _multc3 _divsc3 _divdc3 _divxc3 _divtc3 \
	_bswapsi2 _bswapdi2 _clrsbsi2 _clrsbdi2

.if ${LIBGCC_MACHINE_ARCH} != "coldfire" && empty(LIBGCC_MACHINE_ARCH:Mearm*)

LIB2FUNCS_ALL+= \
	_muldi3 _negdi2 _lshrdi3 _ashldi3 _ashrdi3

.endif

.if ${LIBGCC_MACHINE_ARCH} != "coldfire" && \
    empty(LIBGCC_MACHINE_ARCH:Mearm*) && \
    empty(LIBGCC_MACHINE_ARCH:Maarch64*) && \
    empty(LIBGCC_MACHINE_ARCH:Mmips64*) && \
    empty(LIBGCC_MACHINE_ARCH:Mmipsn64*)

EXTRA_FUNC_SIZES=	sf df xf

.if ${LIBGCC_MACHINE_ARCH} != "x86_64" && \
    ${LIBGCC_MACHINE_ARCH} != "i386" && \
    ${LIBGCC_MACHINE_ARCH} != "ia64"
EXTRA_FUNC_SIZES+=	tf
.endif

# non swfloat versions 
.for _p in _fix _fixuns
. for _m in ${EXTRA_FUNC_SIZES}
LIB2FUNCS_ALL+=	${_p}${_m}di
. endfor
.endfor

.for _p in _float _floatun
. for _m in ${EXTRA_FUNC_SIZES}
LIB2FUNCS_ALL+=	${_p}di${_m}
. endfor
.endfor

.for _m in sf df xf
LIB2FUNCS_ALL+=	_fixuns${_m}si
.endfor
.endif

LIB2FUNCS_SHORT:=${LIB2FUNCS_ALL}
LIB2_DIVMOD_FUNCS:=${G_LIB2_DIVMOD_FUNCS}
.for f in ${G_LIB1ASMFUNCS}; do
LIB2FUNCS_SHORT:=${LIB2FUNCS_SHORT:N${f}}
LIB2_DIVMOD_FUNCS:=${LIB2_DIVMOD_FUNCS:N${f}}
.endfor

LIB2FUNCS=	${LIB2FUNCS_SHORT:=.c}
_LIB2FUNCS=	${G_LIB2FUNCS_ST}
.for _f in ${LIB2FUNCS_EXCLUDE}
_LIB2FUNCS=	${_LIB2FUNCS:N${_f}}
.endfor
LIB2FUNCS_ST=	${_LIB2FUNCS:=.c}
LIB2DIVMOD=	${LIB2_DIVMOD_FUNCS:=.c}
.if ${HAVE_LIBGCC_EH} == "no"
LIB2_EH=	${G_LIB2ADDEH:M*.c:T:Nunwind-dw2.c:Nunwind-dw2-fde-dip.c:Nunwind-sjlj.c:Nunwind-c.c:Nunwind-arm.c:Npr-support.c}
LIB2_EHASM=	${G_LIB2ADDEH:M*.S:T:Nlibunwind.S}
.else
LIB2_EH=	${G_LIB2ADDEH:M*.c:T}
LIB2_EHASM=	${G_LIB2ADDEH:M*.S:T}

INCSDIR=	/usr/include
# originally copied from unwind-*.h
INCS+=		unwind.h
.endif

# We have to weed out any existing func.S file that may exist
# from the list of files we create.
.if !empty(G_LIB1ASMFUNCS)
. for f in ${G_LIB1ASMFUNCS:=.S}
.  if !exists(${DIST}/libgcc/config/${GCC_MACHINE_SUBDIR}/${f})
LIB1ASMFUNCS+=	${f}
.  endif
. endfor
.endif

.if exists(${.CURDIR}/../arch/${LIBGCC_MACHINE_ARCH}/defs.mk)
DPSRCS+=	${.CURDIR}/../arch/${LIBGCC_MACHINE_ARCH}/defs.mk
.endif
CLEANFILES+=	${LIB2FUNCS} ${LIB2FUNCS_ST} ${LIB2DIVMOD} cs-tconfig.h
.if !empty(G_LIB1ASMFUNCS)
CLEANFILES+=	${LIB1ASMFUNCS}
.endif

.for file in ${G_LIB2ADD:M${GNUHOSTDIST}*:N*.c}
BUILDSYMLINKS+=	${file} ${file:T:S/.asm$/.S/}
.endfor

# XXX
.if (${LIBGCC_MACHINE_ARCH} == "m68000" || ${LIBGCC_MACHINE_ARCH} == "m68k" || \
     ${LIBGCC_MACHINE_ARCH} == "coldfire")
CPICFLAGS:=
BUILDSYMLINKS+=	${DIST}/libgcc/config/m68k/fpgnulib.c fpgnulib.c
BUILDSYMLINKS+=	${DIST}/libgcc/config/m68k/fpgnulib.c xfgnulib.c
CPPFLAGS.xfgnulib.c+=	-DEXTFLOAT=1
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

.if ${LIB} == "gcc" && \
    (${LIBGCC_MACHINE_ARCH} == "aarch64" || \
     ${LIBGCC_MACHINE_ARCH} == "aarch64eb") # {
# XXX lse.S objects are placed directly into "libgcc-objects" for GCC, and
# are not pulled out by mknative-gcc easily.
. for s in 1 2 4 8 16
.  for m in 1 2 3 4 5
.   for n in cas
LSE_NAMES+=			${n}_${s}_${m}
CPPFLAGS.${n}_${s}_${m}+=	-DL_${n} -DSIZE=${s} -DMODEL=${m}
.   endfor
.  endfor
. endfor

. for s in 1 2 4 8
.  for m in 1 2 3 4 5
.   for n in swp ldadd ldclr ldeor ldset
LSE_NAMES+=			${n}_${s}_${m}
CPPFLAGS.${n}_${s}_${m}+=	-DL_${n} -DSIZE=${s} -DMODEL=${m}
.   endfor
.  endfor
. endfor

LSE_SRC=	${GNUHOSTDIST}/libgcc/config/aarch64/lse.S

${LSE_NAMES:=.o}: ${LSE_SRC} Makefile
	${_MKTARGET_COMPILE}
	${COMPILE.S} ${CPPFLAGS.${.TARGET:T:.o=}} -o ${.TARGET} ${LSE_SRC}

${LSE_NAMES:=.go}: ${LSE_SRC} Makefile
	${_MKTARGET_COMPILE}
	${COMPILE.S} ${DEBUGFLAGS} ${CPPFLAGS.${.TARGET:T:.go=}} -o ${.TARGET} \
		${LSE_SRC}

# lse-init.c currently compiles to nothing for us
G_LIB2ADD+=	${GNUHOSTDIST}/libgcc/config/aarch64/lse-init.c

.endif # } arm64

# Use dynamic sources to arrange for the C file to be the first dependency
# so ${.IMPSRC} will work.

${LIB2_EH:.c=.o}: $(.PREFIX).c
	${_MKTARGET_COMPILE}
	${COMPILE.c} -fexceptions ${COPTS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC}

${LIB2_EH:.c=.pico}: $(.PREFIX).c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -fexceptions ${COPTS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC}

${G_LIB2_DIVMOD_FUNCS:=.o}: $(.PREFIX).c
	${_MKTARGET_COMPILE}
	${COMPILE.c} -fexceptions -fnon-call-exceptions ${COPTS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC}

${G_LIB2_DIVMOD_FUNCS:=.pico}: $(.PREFIX).c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -fexceptions -fnon-call-exceptions ${COPTS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC}

CPPFLAGS.fp-bit.c+=	-DFLOAT=1
dp-bit.c: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#include <fp-bit.c>\n' >${.TARGET}

#
# Common rules.
#

.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/cp/inc ${DIST}/gcc/config \
	${DIST}/libgcc/config ${DIST}/libgcc/config/${GCC_MACHINE_SUBDIR} \
	${G_LIB2ADD:H:O:u} ${G_LIB2ADDEH:H:O:u} ${GCCARCHLIBGCC}

.include "../Makefile.tconfigh"
DPSRCS+=	tconfig.h

.include "../Makefile.gthr"

EXTRA_FAKEHEADERS=	options.h libgcc_tm.h
.include "../../Makefile.hacks"

# XXX insn-modes.h
BINBACKENDOBJ!=	cd ${.CURDIR}/../../../usr.bin/backend && ${PRINTOBJDIR}
CPPFLAGS+=	-I${BINBACKENDOBJ}

#.if !empty(LIBGCC_MACHINE_ARCH:Mearm*)
COPTS.unwind-dw2.c+=	-Wno-discarded-qualifiers
#.endif
