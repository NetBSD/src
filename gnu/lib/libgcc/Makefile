#	$NetBSD: Makefile,v 1.40 2002/03/26 23:10:58 fredette Exp $

REQUIRETOOLS=	yes

LIB=		gcc
NOLINT=		# defined

.include <bsd.own.mk>

.cc: # disable .cc->NULL transform

.if defined(USE_NEW_TOOLCHAIN)

.if exists(${.CURDIR}/${MACHINE_ARCH}.mk)
.include "${.CURDIR}/${MACHINE_ARCH}.mk"

TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain
GCCARCH=	${TOP}/usr.bin/gcc/arch/${MACHINE_ARCH}

GCPPFLAGS=	${G_LIBGCC2_CFLAGS} ${G_MAYBE_USE_COLLECT2} ${G_INCLUDES}
CPPFLAGS+=	-I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*} \
		-I${DIST}/gcc/cp

.if (${MACHINE_ARCH} == "sparc64")
CFLAGS+=        -msoft-quad-float
.endif

LIB2FUNCS=	${G_LIB2FUNCS:=.c}
.if defined(G_LIB2FUNCS_EH)
LIB2FUNCS_EH=	${G_LIB2FUNCS_EH:=.c}
.endif
CXX_LIB2FUNCS=	${G_CXX_LIB2FUNCS:Mop*:S/opdel.o/opdelete.o/:.o=.cc}

SRCS+=		${LIB2FUNCS} ${LIB2FUNCS_EH} ${CXX_LIB2FUNCS} \
		${G_CXX_LIB2FUNCS:Nop*:.o=.cc} ${G_LIB2ADD:M*.c:T}
OBJS+=		${G_LIB2FUNCS_EH:=.o}

# XXX fp-bit.c should be removed from this list after some grace period.
CLEANFILES+=	${LIB2FUNCS} ${CXX_LIB2FUNCS} dp-bit.c _eh.c fp-bit.c

${LIB2FUNCS}: ${.CURDIR}/Makefile
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

.if defined(G_LIB2FUNCS_EH)
${LIB2FUNCS_EH}: ${.CURDIR}/Makefile
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

${G_LIB2FUNCS_EH:=.o}:
	${COMPILE.c} -fexceptions -o ${.TARGET} ${.IMPSRC}

${G_LIB2FUNCS_EH:=.po}:
	${COMPILE.c} -pg -fexceptions -o ${.TARGET} ${.IMPSRC}

${G_LIB2FUNCS_EH:=.so}:
	${COMPILE.c} ${CPICFLAGS} -fexceptions -o ${.TARGET} ${.IMPSRC}
.endif	# G_LIB2FUNCS_EH

${CXX_LIB2FUNCS}: ${.CURDIR}/Makefile
	printf '#define L_op_${.PREFIX:S/^op//}\n#include <new1.cc>\n#include <new2.cc>\n' >${.TARGET}

dp-bit.c: ${.CURDIR}/Makefile
	printf '#define FLOAT\n#include <fp-bit.c>\n' >${.TARGET}

INCS=		exception new new.h typeinfo
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/cp/inc ${DIST}/gcc/config \
	${G_CONFIGDIR}

.include <bsd.lib.mk>

${OBJS} ${SOBJS} ${POBJS}: ${.CURDIR}/${MACHINE_ARCH}.mk
.else
.include <bsd.prog.mk> # do nothing
.endif

.else
#
# XXX - BEGIN - Old toolchain build goop - delete when everything migrated - XXX
#
HAVE_GCC28!=	${CXX} --version | egrep "^(2\.8|egcs)" ; echo

.if !empty(HAVE_GCC28)
MKPIC:=		no
MKPROFILE:=	no

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
    (${MACHINE_ARCH} == "arm")
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
