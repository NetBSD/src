#	$NetBSD: Makefile,v 1.7 1999/02/13 02:54:23 lukem Exp $

HAVE_GCC28!=	${CXX} --version | egrep "^(2\.8|egcs)" ; echo

LIB=		gcc
MKLINT=		no
MKPROFILE=	no
MKPIC=		no
DIST=		${.CURDIR}/../../dist/gcc
ARCH=		${.CURDIR}/../../usr.bin/egcs/arch

CPPFLAGS+=	-I${ARCH}/${MACHINE_ARCH} -I${ARCH} \
		-I${DIST}/config -I${DIST}/cp -I${DIST}

LIB2OBJS=	__gcc_bcmp.o _varargs.o __dummy.o _eprintf.o _bb.o _shtab.o \
		_clear_cache.o _trampoline.o __main.o _exit.o _ctors.o \
		_pure.o
.if (${MACHINE_ARCH} == "m68k") || (${MACHINE_ARCH} == "i386") || \
    (${MACHINE_ARCH} == "sparc64")
LIB2OBJS+=	_fixxfdi.o _fixunsxfdi.o _floatdixf.o _fixunsxfsi.o
.endif
OBJS+=		${LIB2OBJS}

# Extra code to include in libgcc2 for C++ support.
.if !empty(HAVE_GCC28)
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
.endif

${LIB2OBJS}: libgcc2.c
	${COMPILE.c} -DL$* -o $@ $>

afterinstall:
	@if [ ! -f _eh.o ]; then\
		echo '***** You should rebuild and reinstall libgcc.a using the new egcs'; \
		echo '***** compiler in order to include all the necessary C++ support.'; \
	fi

.PATH:		${DIST} ${DIST}/cp ${DIST}/cp/inc

.include <bsd.lib.mk>
