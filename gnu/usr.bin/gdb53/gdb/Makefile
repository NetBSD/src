#	$NetBSD: Makefile,v 1.5 2003/09/26 17:51:18 christos Exp $

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

LDADD=		-lcurses -ltermcap -lintl -lm -lkvm
DPADD=		${LIBCURSES} ${LIBTERMCAP} ${LIBINTL} ${LIBM} ${LIBKVM}

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
#.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
#PSIMOBJ!=	cd ${TOP}/usr.bin/sim/ppc && ${PRINTOBJDIR}
#LDADD+=		-L${PSIMOBJ} -lsim
#DPADD+=		${PSIMOBJ}/libsim.a
#.endif

LDADD+= -L${LIBOPCODESDIR} -lopcodes\
	-L${LIBBFDDIR} -lbfd \
	${LIBEDIT} \
	-L${LIBIBERTYDIR} -liberty \


.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/signals ${DIST}/gdb/doc ${.CURDIR}/../arch/${MACHINE_ARCH}
