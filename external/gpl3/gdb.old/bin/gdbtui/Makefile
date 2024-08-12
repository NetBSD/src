#	$NetBSD: Makefile,v 1.11 2024/08/12 20:30:05 christos Exp $

NOCTF=
NOMAN=
.include <bsd.own.mk>
.include "../Makefile.inc"

GDBLIBDIR=${.CURDIR}/../../lib
GDBARCHDIR=${GDBLIBDIR}/libgdb/arch/${GDB_MACHINE_ARCH}
.include "${GDBARCHDIR}/defs.mk"

PROG=		gdbtui

SRCS=		tui-main.c 
CPPFLAGS+=	-I${.CURDIR} \
		-I${GNULIBDIR}/libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib \
		-I${DIST}/gdb \
		-I${DIST}/gdbsupport \
		-I${DIST}/gdb/config \
		-I${DIST}/gnulib/import \
		-I${DIST}/include/opcode \
		-I${DIST}/readline \
		-I${DIST}/libdecnumber \
		-I${DIST} \
		-I${GDBARCHDIR} \
		-I${GDBLIBDIR}/libgdbsupport/arch/${GDB_MACHINE_ARCH} \
		-I${GDBLIBDIR}/libbfd/arch/${GDB_MACHINE_ARCH} \
		-I${GDBLIBDIR}/libgnulib/arch/${GDB_MACHINE_ARCH} \
		-I${GDBLIBDIR}/libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib/import \
		-I${DIST}/bfd \
		-I${DIST}/include \
		${GCPPFLAGS:M-D*}

.include "../../Makefile.inc"
.include <bsd.info.mk>

PROGDPLIBS+= gdb ${GDBLIBDIR}/libgdb
# XXX: modula2 won't load otherwise, since nothing brings the m2-objs in.
LIBGDBDIR!=	cd ${GDBLIBDIR}/libgdb && ${PRINTOBJDIR}
LDADD+=		${LIBGDBDIR}/m2-lang.o
#LDADD+= -L${LIBGDBDIR} -Wl,--whole-archive -lgdb -Wl,-no-whole-archive
#DPADD+= ${LIBGDBDIR}/libgdb.a

# Simulator support
.if ${GDB_MACHINE_ARCH} == powerpc || ${GDB_MACHINE_ARCH} == powerpcle || \
    ${GDB_MACHINE_ARCH} == powerpc64
PROGDPLIBS+= sim ${GDBLIBDIR}/libsim
.elif ${MACHINE_CPU} == xxmips
PROGDPLIBS+= sim ${GDBLIBDIR}/libsim
.endif

PROGDPLIBS+= opcodes ${GDBLIBDIR}/libopcodes \
    bfd ${GDBLIBDIR}/libbfd \
    decnumber ${GDBLIBDIR}/libdecnumber \
    gdbsupport ${GDBLIBDIR}/libgdbsupport \
    backtrace ${GDBLIBDIR}/libbacktrace \
    sframe ${GDBLIBDIR}/libsframe \
    ctf ${GDBLIBDIR}/libctf \
    gnulib ${GDBLIBDIR}/libgnulib \
    iberty ${GDBLIBDIR}/libiberty

.if ${USE_LIBEDIT} == yes
LDADD+= -ledit
DPADD+= ${LIBEDIT}
.else
PROGDPLIBS+= readline ${GDBLIBDIR}/libreadline
.endif

.include <bsd.prog.mk>

LIBGMP!=	cd ${.CURDIR}/../../../../lgpl3/gmp/lib/libgmp && ${PRINTOBJDIR}
LDADD+= -L${LIBGMP} -lgmp
DPADD+= ${LIBGMP}/libgmp.a

LDADD+=	-lexpat -llzma -lz -lcurses -lterminfo -lintl -lm -lkvm -lutil -lpthread
DPADD+=	${LIBEXPAT} ${LIBLZMA} ${LIBZ} ${LIBCURSES} ${LIBTERMINFO} ${LIBINTL} \
    ${LIBM} ${LIBKVM} ${LIBUTIL} ${LIBPTHREAD}

# Force c++
CWARNFLAGS.clang+=	-Wno-deprecated
HOST_CC:=		${HOST_CXX}
HOST_CFLAGS+=		${HOST_CXXFLAGS}
CC:=			${CXX}
CFLAGS:=		${CXXFLAGS} -std=gnu++11 -Wno-error=stack-protector

.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/signals ${DIST}/gdb/syscalls ${DIST}/gdb/doc \
    ${.CURDIR}/../arch/${GDB_MACHINE_ARCH}
