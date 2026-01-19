#	$NetBSD: Makefile,v 1.2 2026/01/19 09:26:17 mrg Exp $

# XXX libgfortran also has a libcaf_single thing in here.

# This needs to be before bsd.init.mk
.if defined(BSD_MK_COMPAT_FILE)
.include <${BSD_MK_COMPAT_FILE}>
.endif

.include <bsd.init.mk>

LIB=		gfortran

# Match GCC 14.
SHLIB_MAJOR=	5
SHLIB_MINOR=	0

DIST=		${GCCDIST}
GNUHOSTDIST=    ${DIST}
GFORTRAN=	${DIST}/libgfortran

# These are earlier than included file paths, to avoid getting eg
# libbacktrace's read.c instead of io/read.c.
.PATH: ${DIST}/libgfortran
.PATH: ${DIST}/libgfortran/generated
.PATH: ${DIST}/libgfortran/intrinsics
.PATH: ${DIST}/libgfortran/runtime
.PATH: ${DIST}/libgfortran/io
.PATH: ${DIST}/libgfortran/ieee
.PATH: ${.CURDIR}/arch/${GCC_MACHINE_ARCH}

# remove prefixes, .inc and .h files
GFORTRAN_SRCS=	${G_libgfortran_la_SOURCES:C,^.*/,,:C,.*\.inc$,,:C,.*\.h$,,}

.include "../Makefile.gthr"
.include "${.CURDIR}/arch/${GCC_MACHINE_ARCH}/defs.mk"
.include "../libbacktrace/Makefile.sources"

GFORTRAN_BACKTRACE_SRCS=	${BACKTRACE_SRCS:backtrace.c=libbacktrace-backtrace.c}

BUILDSYMLINKS+= ${GNUHOSTDIST}/libbacktrace/backtrace.c libbacktrace-backtrace.c

# gcc builddir before libgcc
# libgcc builddir after libgcc

.if ${HAVE_QUADMATH} != no
CPPFLAGS_QUADMATH=-I${DIST}/libquadmath
.else
CPPFLAGS_QUADMATH=
.endif

.for _s in ${GFORTRAN_SRCS}
CPPFLAGS.${_s}+=	-I. \
		-I${.CURDIR}/arch/${GCC_MACHINE_ARCH} \
		-I${GFORTRAN} \
		-I${GFORTRAN}/io \
		-I${DIST}/gcc \
		-I${DIST}/gcc/config \
		${CPPFLAGS_QUADMATH} \
		-I${DIST}/libgcc \
		-I${DIST}/libbacktrace \
		-I${DIST}/../lib/libbacktrace/arch/${GCC_MACHINE_ARCH} \
		-I${.CURDIR}/arch/${GCC_MACHINE_ARCH}
.endfor

.for _s in ${GFORTRAN_BACKTRACE_SRCS}
CPPFLAGS.${_s}+=	-I. \
		${BACKTRACE_CPPFLAGS}
.endfor

SRCS=		${GFORTRAN_SRCS} ${GFORTRAN_BACKTRACE_SRCS}

FFLAGS+=	-I. -fno-repack-arrays -fno-underscoring -I${.CURDIR}/arch/${GCC_MACHINE_ARCH}

LIBDPLIBS+=	m ${NETBSDSRCDIR}/lib/libm
.if ${HAVE_QUADMATH} != no
LIBDPLIBS+=	quadmath ${.CURDIR}/../libquadmath
.endif

DPSRCS+=	kinds.h kinds.inc c99_protos.inc fpu-target.inc \
		selected_int_kind.inc selected_real_kind.inc

${GFORTRAN_SRCS:M*.F90}: ${DPSRCS}

INTLIST='1 2 4 8 16'
REALLIST='4 8 10 16'

fpu-target.h: ${GFORTRAN}/${G_FPU_HOST_HEADER}
	cp ${GFORTRAN}/${G_FPU_HOST_HEADER} $@

fpu-target.inc: fpu-target.h ${DIST}/gcc/fortran/libgfortran.h
	${TOOL_GREP} -h '^#define GFC_FPE_' ${.ALLSRC} > ${.TARGET}

c99_protos.inc: ${GFORTRAN}/c99_protos.h
	${TOOL_GREP} '^#' < ${.ALLSRC} > ${.TARGET} || rm ${.TARGET}
LIBS+=	${BACKENDOBJ}/libbackend.a
kinds.h: ${GFORTRAN}/mk-kinds-h.sh
	${HOST_SH} ${GFORTRAN}/mk-kinds-h.sh \
		    ${INTLIST} \
		    ${REALLIST} \
		    "${FC} ${FFLAGS}" \
		    no > \
	    ${.TARGET} || rm ${.TARGET}

kinds.inc: kinds.h ${GFORTRAN}/kinds-override.h
	cat kinds.h ${GFORTRAN}/kinds-override.h | \
		    ${TOOL_GREP} '^#' | \
		    ${TOOL_GREP} -v include > \
	    ${.TARGET} || rm ${.TARGET}

selected_int_kind.inc: ${GFORTRAN}/mk-sik-inc.sh
	${HOST_SH} ${GFORTRAN}/mk-sik-inc.sh \
		    ${INTLIST} \
		    "${FC} ${FFLAGS}" > \
	    ${.TARGET} || rm ${.TARGET}

selected_real_kind.inc: ${GFORTRAN}/mk-srk-inc.sh
	${HOST_SH} ${GFORTRAN}/mk-srk-inc.sh \
		    ${REALLIST} \
		    "${FC} ${FFLAGS}" > \
	    ${.TARGET} || rm ${.TARGET}

fpu.c: fpu-target.h

COPTS.date_and_time.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.list_read.c+=	-Wno-error=pointer-arith
COPTS.transfer.c+=	-Wno-error=pointer-arith
COPTS.write.c+=		-Wno-error=pointer-arith

# ieee_arithmetic needs ieee_exceptions.mod to be already built
ieee_arithmetic.o ieee_arithmetic.pico ieee_arithmetic.po ieee_arithmetic.go: ieee_exceptions.pico ieee_exceptions.po ieee_exceptions.o ieee_exceptions.go

# these .mod's gets built multiple times, so serialise those
.for _mod in ieee_arithmetic ieee_exceptions ieee_features
${_mod}.mod: ${_mod}.o
${_mod}.o: ${_mod}.pico
${_mod}.pico: ${_mod}.po
${_mod}.po: ${_mod}.go
.endfor

CLEANFILES+=	fpu-target.h fpu-target.inc c99_protos.inc \
		kinds.h kinds.inc \
		selected_int_kind.inc selected_real_kind.inc \
		ieee_arithmetic.mod ieee_exceptions.mod ieee_features.mod

.for _f in ${G_i_matmul_c:C,^.*/,,}
FOPTS.${_f}+=	-ffast-math -ftree-vectorize -funroll-loops --param max-unroll-times=4
.endfor

#.if ${MACHINE} == "amd64"
.for _f in ${G_i_matmulavx128_c:C,^.*/,,}
FOPTS.${_f}+=	-ffast-math -ftree-vectorize -funroll-loops --param max-unroll-times=4 -mprefer-avx128
.endfor
#.endif

.for _f in ${G_i_matmull_c:C,^.*/,,}
FOPTS.${_f}+=	-funroll-loops
.endfor

.for _f in ${G_gfor_specific_src:C,^.*/,,}
FOPTS.${_f}+=	-fallow-leading-underscore -fbuilding-libgfortran
.endfor

.for _f in selected_real_kind.f90 selected_int_kind.f90
FOPTS.${_f}+=	-fallow-leading-underscore -fbuilding-libgfortran
.endfor

.for _f in ${G_gfor_ieee_src:C,^.*/,,}
FOPTS.${_f}+=	-Wno-unused-dummy-argument -Wno-c-binding-type -ffree-line-length-0 -fallow-leading-underscore -fsignaling-nans -fbuilding-libgfortran
.endfor

.for _f in ${G_gfor_ieee_helper_src:C,^.*/,,}
FOPTS.${_f}+=	-fsignaling-nans
.endfor

# Don't install these more than once.
.ifndef MLIBDIR
INCS=		${G_nodist_finclude_HEADERS}
INCSDIR=	${GCC_INCSDIR}/finclude

# This is where GCC looks for it.
FILES=		libgfortran.spec
FILESDIR=	${LIBDIR}
.endif

.if ${MACHINE} == "vax"
. for _f in unpack_i1.c unpack_i2.c unpack_i4.c unpack_i8.c unpack_r4.c unpack_r8.c unpack_c4.c unpack_c8.c
COPTS.${_f}+=	${CC_WNO_MAYBE_UNINITIALIZED}
. endfor
# Ugh infinity/nan stuff
. for _f in read.c erfc_scaled.c c99_functions.c trigd.c
COPTS.${_f}+=	-Wno-error
. endfor
.endif

# XXX
.if ${MACHINE_ARCH} == "riscv32"
COPTS.ISO_Fortran_binding.c+=	-Wno-error=format
.endif

.include <bsd.lib.mk>
