#	$NetBSD: Makefile,v 1.3 2020/09/24 14:53:35 christos Exp $

NOCTF=
NOMAN=
.include <bsd.own.mk>
.include "../Makefile.inc"

PROG=		gdbserver

SERVER_SRCS+=	ax.cc debug.cc dll.cc hostio.cc hostio-errno.cc inferiors.cc mem-break.cc notif.cc
SERVER_SRCS+=	regcache.cc remote-utils.cc server.cc symbol.cc target.cc
SERVER_SRCS+=	tdesc.cc tracepoint.cc utils.cc waitstatus.c version.cc

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

LIBBFD!=	     cd ${.CURDIR}/../../lib/libbfd	   && ${PRINTOBJDIR}
LIBGNULIBDIR!=	     cd ${.CURDIR}/../../lib/libgnulib	   && ${PRINTOBJDIR}
LIBGDBSUPPORTDIR!=   cd ${.CURDIR}/../../lib/libgdbsupport && ${PRINTOBJDIR}
LIBIBERTYDIR!=	     cd ${.CURDIR}/../../lib/libiberty	   && ${PRINTOBJDIR}

LDADD+= -L${LIBGDBSUPPORTDIR} -lgdbsupport \
	-L${LIBGNULIBDIR} -lgnulib \
	-L${LIBIBERTYDIR} -liberty \
	-lintl

DPADD+=	${LIBGDBSUPPORTDIR}/libgdbsupport.a \
	${LIBGNULIBDIR}/libgnulib.a \
	${LIBIBERTYDIR}/libiberty.a \
	${LIBINTL}

.include <bsd.prog.mk>

# Force c++
CWARNFLAGS.clang+=	-Wno-deprecated
HOST_CC:=		${HOST_CXX}
CC:=			${CXX}
CFLAGS:=		${CXXFLAGS} -std=gnu++11 -Wno-error=stack-protector -pthread

.PATH: ${DIST}/gdbserver ${DIST}/gdb/nat ${DIST}/gdb/arch ${DIST}/gdb/target \
	${.CURDIR}/arch/${GDB_MACHINE_ARCH} \
	${DIST}/../lib/libgdbsupport/arch/${GDB_MACHINE_ARCH}
