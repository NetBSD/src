#	$NetBSD: Makefile,v 1.3 2003/08/17 18:03:28 christos Exp $

.include <bsd.own.mk>

.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)
MKPIC=no
MKPROFILE=no  

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

PROG=		gdb

SRCS=		${G_GDB_OBJS:.o=.c}
CPPFLAGS=	-I${.CURDIR}/../arch/${MACHINE_ARCH} \
		-I${DIST}/gdb \
		-I${DIST}/gdb/config \
		-I${DIST}/include \
		-I${DIST}/include/opcode \
		-I${DIST} \
		-I${DIST}/bfd \
		-I${.CURDIR}/../bfd \
		-I${DIST}/gdb/tui \
		-DHAVE_CONFIG_H \
		-DLOCALEDIR='"/usr/share/locale"' \
		-DUSE_INCLUDED_REGEX \
		-DRL_LIBRARY_VERSION='"4.0"' \
		-DNETBSD_CORE


LDADD=		-lcurses -ltermcap -lintl -lm -lkvm
DPADD=		${LIBTERMCAP} ${LIBINTL} ${LIBM} ${LIBKVM}

.if ${SRCS:Mnbsd-thread.c} != ""
LDADD+=                -lpthread_dbg
.endif

TEXINFO=	gdb.texinfo gdbint.texinfo stabs.texinfo
INFOFLAGS=	-I${DIST}/gdb/doc -I${DIST}/gdb/mi \
		-I${DIST}/readline/doc -I${.CURDIR}/..

.endif

.include "../../Makefile.inc"
.include <bsd.prog.mk>
.include <bsd.info.mk>

LIBOPCODESDIR!=  cd ${.CURDIR}/../opcodes && ${PRINTOBJDIR}
LIBBFDDIR!=  cd ${.CURDIR}/../bfd && ${PRINTOBJDIR}
LIBREADLINEDIR!=  cd ${.CURDIR}/../readline && ${PRINTOBJDIR}
LIBIBERTYDIR!=  cd ${.CURDIR}/../libiberty && ${PRINTOBJDIR}

# PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
PSIMOBJ!=	cd ${TOP}/usr.bin/sim/ppc && ${PRINTOBJDIR}
LDADD+=		-L${PSIMOBJ} -lsim
DPADD+=		${PSIMOBJ}/libsim.a
.endif

LDADD+= -L${LIBOPCODESDIR} -lopcodes\
	-L${LIBBFDDIR} -lbfd \
	-L${LIBREADLINEDIR} -lreadline \
	-L${LIBIBERTYDIR} -liberty

.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/signals ${DIST}/gdb/doc ${.CURDIR}/../arch/${MACHINE_ARCH}
