#	$NetBSD: Makefile,v 1.7 2004/04/02 13:36:33 mrg Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"

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
		-I${DIST}/bfd \
		-I${.CURDIR}/../bfd \
		-I${DIST}/gdb/tui \
		-DHAVE_CONFIG_H \
		-DLOCALEDIR='"/usr/share/locale"' \
		-DUSE_INCLUDED_REGEX \
		-DNETBSD_CORE

.if ${USE_TUI} == "yes"
SRCS+=		${G_TUI_OBJS:.o=.c}
CPPFLAGS+=	-DUSE_TUI
.endif

LDADD=		-lcurses -lintl -lm -lkvm
DPADD=		${LIBCURSES} ${LIBINTL} ${LIBM} ${LIBKVM}

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
LIBIBERTYDIR!=  cd ${.CURDIR}/../libiberty && ${PRINTOBJDIR}


.if ${USE_LIBEDIT} == yes && ${USE_TUI} == no
LIBEDIT=-ledit
.else
CPPFLAGS+= -DRL_LIBRARY_VERSION='"4.0"' -I${DIST}
LIBREADLINEDIR!=  cd ${.CURDIR}/../readline && ${PRINTOBJDIR}
LIBEDIT= -L${LIBREADLINEDIR} -lreadline
.endif

## PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
PSIMOBJ!=	cd ${.CURDIR}/../sim/ppc && ${PRINTOBJDIR}
LDADD+=		-L${PSIMOBJ} -lsim
DPADD+=		${PSIMOBJ}/libsim.a
.endif

LDADD+= -L${LIBOPCODESDIR} -lopcodes\
	-L${LIBBFDDIR} -lbfd \
	${LIBEDIT} \
	-ltermcap \
	-L${LIBIBERTYDIR} -liberty \

DPADD+= ${LIBOPCODESDIR}/libopcodes.a \
	${LIBBFDDIR}/libbfd.a \
	${LIBEDIT} \
	${LIBTERMCAP} \
	${LIBIBERTYDIR}/libiberty.a \


.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/signals ${DIST}/gdb/doc ${.CURDIR}/../arch/${MACHINE_ARCH}
