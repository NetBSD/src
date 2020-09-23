#	$NetBSD: Makefile,v 1.1 2020/09/23 23:36:59 christos Exp $

NOCTF=
NOMAN=
.include <bsd.own.mk>
.include "../Makefile.inc"

PROG=		gdbserver

SERVER_SRCS=		server.cc
NETBSD_SRCS=		netbsd-low.cc fork-child.cc fork-inferior.c netbsd-nat.c
ARCH_SRCS=		netbsd-${MACHINE}-low.cc ${MACHINE}.c
SRCS=		${SERVER_SRCS} ${NETBSD_SRCS} ${ARCH_SRCS}
CPPFLAGS+=	-I${.CURDIR} \
		-I${.CURDIR}/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../../lib/libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib \
		-I${DIST}/gdb \
		-I${DIST}/gdbserver \
		-I${DIST}/gdbsupport \
		-I${DIST}/gdb/regformats \
		-I${DIST}/gdb/config \
		-I${DIST}/gnulib/import \
		-I${DIST} \
		-I${.CURDIR}/../../lib/libgdbsupport/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../../lib/libgnulib/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../../lib/libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib/import \
		-I${DIST}/include \
		${GCPPFLAGS:M-D*}

.include "../../Makefile.inc"
.include <bsd.info.mk>

LIBGDB!=	     cd ${.CURDIR}/../../lib/libgdb	   && ${PRINTOBJDIR}
LIBBFD!=	     cd ${.CURDIR}/../../lib/libbfd	   && ${PRINTOBJDIR}
LIBGNULIBDIR!=	     cd ${.CURDIR}/../../lib/libgnulib	   && ${PRINTOBJDIR}
LIBGDBSUPPORTDIR!=   cd ${.CURDIR}/../../lib/libgdbsupport && ${PRINTOBJDIR}
LIBIBERTYDIR!=	     cd ${.CURDIR}/../../lib/libiberty	   && ${PRINTOBJDIR}

LDADD+= -L${LIBGDB} -lgdb \
	-L${LIBGDBSUPPORTDIR} -lgdbsupport \
	-L${LIBGNULIBDIR} -lgnulib \
	-L${LIBBFD} -lbfd \
	-L${LIBIBERTYDIR} -liberty \
	-lintl -lz

DPADD+=	${LIBGDB}/libgdb.a \
	${LIBGDBSUPPORTDIR}/libgdbsupport.a \
	${LIBGNULIBDIR}/libgnulib.a \
	${LIBBFD}/libbfd.a \
	${LIBIBERTYDIR}/libiberty.a \
	${LIBINTL} ${LIBZ}

.include <bsd.prog.mk>

# Force c++
CWARNFLAGS.clang+=	-Wno-deprecated
HOST_CC:=		${HOST_CXX}
CC:=			${CXX}
CFLAGS:=		${CXXFLAGS} -std=gnu++11 -Wno-error=stack-protector -pthread

.PATH: ${DIST}/gdbserver ${DIST}/gdb/nat ${DIST}/gdb/arch
