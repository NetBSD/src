#	$NetBSD: Makefile,v 1.18 2001/07/18 18:29:02 tv Exp $

.include <bsd.own.mk>

LIB=		gcc
MKLINT=		no
MKPROFILE=	no

.if defined(USE_NEW_TOOLCHAIN)

.include "${MACHINE_ARCH}.mk"

MKPIC=no	# XXX to be fixed

TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain
GCCARCH=	${TOP}/usr.bin/gcc/arch/${MACHINE_ARCH}

SRCS=		${G_LIB2ADD:M*.c*} ${G_CXX_LIB2SRCS:N*/new?.cc}

GCPPFLAGS=	${G_LIBGCC2_CFLAGS} ${G_MAYBE_USE_COLLECT2} ${G_INCLUDES}
CPPFLAGS+=	-I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}

.PATH: ${DIST}/gcc

OBJS+=		${G_LIB2FUNCS:=.o} ${G_LIB2FUNCS_EH:=.o} ${G_CXX_LIB2FUNCS:Mop*}

${G_LIB2FUNCS:=.o}: libgcc2.c
	${COMPILE.c} -DL$* -o $@ ${>:M*.c}

${G_LIB2FUNCS_EH:=.o}: libgcc2.c
	${COMPILE.c} -fexceptions -DL$* -o $@ ${>:M*.c}

.for f in new newnt
op${f}.o: ${G_CXX_LIB2SRCS:M*/new1.cc}
	${COMPILE.c} -DL_op_${f} -o $@ ${>:M*.cc}
.endfor

.for f in vnew vnewnt del delnt vdel vdelnt
op${f}.o: ${G_CXX_LIB2SRCS:M*/new2.cc}
	${COMPILE.c} -DL_op_${f} -o $@ ${>:M*.cc}
.endfor

.include <bsd.lib.mk>

${OBJS}: ${MACHINE_ARCH}.mk

.else
#
# XXX - BEGIN - Old toolchain build goop - delete when everything migrated - XXX
#
HAVE_GCC28!=	${CXX} --version | egrep "^(2\.8|egcs)" ; echo

.if !empty(HAVE_GCC28)
MKPIC=		no

DIST=		${.CURDIR}/../../dist/gcc
ARCH=		${.CURDIR}/../../usr.bin/egcs/arch

.if exists(${ARCH}/${MACHINE_ARCH})
ARCHSUBDIR= ${MACHINE_ARCH}
.elif exists(${ARCH}/${MACHINE_CPU})
ARCHSUBDIR= ${MACHINE_CPU}
.else
.BEGIN:
	@echo no egcs/arch/${MACHINE_ARCH} nor egcs/arch/${MACHINE_CPU} exists
	@false
.endif

CPPFLAGS+=	-I${ARCH}/${ARCHSUBDIR} -I${ARCH} \
		-I${DIST}/config -I${DIST}/cp -I${DIST}

LIB2OBJS=	__gcc_bcmp.o _varargs.o __dummy.o _eprintf.o _bb.o _shtab.o \
		_clear_cache.o _trampoline.o __main.o _exit.o _ctors.o \
		_pure.o
.if (${MACHINE_ARCH} == "m68k") || (${MACHINE_ARCH} == "i386") || \
    (${MACHINE_ARCH} == "sparc64")
LIB2OBJS+=	_fixxfdi.o _fixunsxfdi.o _floatdixf.o _fixunsxfsi.o
.endif
.if (${MACHINE_ARCH} == "vax")
LIB2OBJS+=	_udiv_w_sdiv.o
.endif
.if (${MACHINE_ARCH} == "sh3eb") || (${MACHINE_ARCH} == "sh3el") || \
    (${MACHINE_ARCH} == "arm26") || (${MACHINE_ARCH} == "arm")
LIB2OBJS+=	_fixunsdfsi.o _fixunssfsi.o
.endif
OBJS+=		${LIB2OBJS}

# Extra code to include in libgcc2 for C++ support.
SRCS+=		tinfo.cc tinfo2.cc new.cc exception.cc frame.c
OBJS+=		${NEW1OBJS} ${NEW2OBJS}
CXXFLAGS+=	-I${DIST}/cp/inc
LIB2OBJS+=	_eh.o
NEW1OBJS=	_op_new.o _op_newnt.o
NEW2OBJS=	_op_vnew.o _op_vnewnt.o _op_delete.o _op_delnt.o \
		_op_vdel.o _op_vdelnt.o

INCS=		exception new new.h typeinfo
INCSDIR=	/usr/include/g++

.cc:
	@true

${NEW1OBJS}: new1.cc
	${COMPILE.cc} -DL$* -o $@ $>
${NEW2OBJS}: new2.cc
	${COMPILE.cc} -DL$* -o $@ $>

${LIB2OBJS}: libgcc2.c
	${COMPILE.c} -DL$* -o $@ $>

afterinstall:
	@if [ ! -f _eh.o ]; then\
		echo '***** You should rebuild and reinstall libgcc.a using the new egcs'; \
		echo '***** compiler in order to include all the necessary C++ support.'; \
	fi

.PATH:		${DIST} ${DIST}/cp ${DIST}/cp/inc

.include <bsd.lib.mk>
.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
#
# XXX - END - Old toolchain build goop - delete when everything migrated - XXX
#
.endif
