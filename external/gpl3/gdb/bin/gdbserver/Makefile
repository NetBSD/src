#	$NetBSD: Makefile,v 1.9 2024/08/18 04:03:57 rin Exp $

NOCTF=
.include <bsd.own.mk>
.include "../Makefile.inc"
GDBLIBDIR=${.CURDIR}/../../lib


PROG=		gdbserver

SERVER_SRCS+=	ax.cc debug.cc dll.cc hostio.cc inferiors.cc
SERVER_SRCS+=	notif.cc mem-break.cc
SERVER_SRCS+=	regcache.cc remote-utils.cc server.cc symbol.cc target-ser.cc
SERVER_SRCS+=	tdesc.cc tracepoint.cc utils.cc waitstatus.c version.cc

NETBSD_SRCS=		netbsd-low.cc fork-child.cc fork-inferior.c netbsd-nat.c
ARCH_SRCS=		netbsd-${MACHINE}-low.cc ${MACHINE}.c
SRCS=		${SERVER_SRCS} ${NETBSD_SRCS} ${ARCH_SRCS}
CPPFLAGS+=	-I${.CURDIR} \
		-I${.CURDIR}/arch/${GDB_MACHINE_ARCH} \
		-I${GDBLIBDIR}/libbfd/arch/${GDB_MACHINE_ARCH} \
		-I${GDBLIBDIR}/libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib \
		-I${DIST}/gdb \
		-I${DIST}/gdbserver \
		-I${DIST}/gdbsupport \
		-I${DIST}/gdb/regformats \
		-I${DIST}/gdb/config \
		-I${DIST}/gnulib/import \
		-I${DIST} \
		-I${GDBLIBDIR}/libgdbsupport/arch/${GDB_MACHINE_ARCH} \
		-I${GDBLIBDIR}/libgnulib/arch/${GDB_MACHINE_ARCH} \
		-I${GDBLIBDIR}/libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib/import \
		-I${DIST}/include \
		-include ${DIST}/gdbserver/server.h \
		-D_GLIBCXX_USE_CXX11_ABI=1 \
		-DPROG='"gdbserver"' \
		${GCPPFLAGS:M-D*}

# /usr/src/current/external/gpl3/gdb/bin/gdbserver/../../dist/gdb/nat/fork-inferior.c:497:15: error: 6 enumeration values not handled in switch: 'TARGET_WAITKIND_VFORK_DONE', 'TARGET_WAITKIND_IGNORE', 'TARGET_WAITKIND_NO_HISTORY'... [-Werror,-Wswitch]
CXXFLAGS+=	${${ACTIVE_CC} == "clang":? -Wno-error=switch : }

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
HOST_CFLAGS+=		${HOST_CXXFLAGS}
CC:=			${CXX}
CFLAGS:=		${CXXFLAGS} -std=gnu++17 -Wno-error=stack-protector -pthread

.PATH: ${DIST}/gdbserver ${DIST}/gdb/nat ${DIST}/gdb/arch ${DIST}/gdb/doc \
	${.CURDIR}/arch/${GDB_MACHINE_ARCH} ${DIST}/gdb/target \
	${DIST}/../lib/libgdbsupport/arch/${GDB_MACHINE_ARCH}
