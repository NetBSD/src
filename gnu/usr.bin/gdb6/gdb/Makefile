#	$NetBSD: Makefile,v 1.5 2006/10/21 21:05:57 apb Exp $

.include "../Makefile.inc"

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb6

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

PROG=		gdb

SRCS=		gdb.c
CPPFLAGS=	-I${.CURDIR} \
		-I${.CURDIR}/../arch/${MACHINE_ARCH} \
		-I${DIST}/gdb \
		-I${DIST}/gdb/config \
		-I${DIST}/include/opcode \
		-I${DIST} \
		-I${.CURDIR}/../bfd/arch/${MACHINE_ARCH} \
		-I${DIST}/bfd \
		-I${DIST}/include \
		${GCPPFLAGS:M-D*}

TEXINFO=	gdb.texinfo gdbint.texinfo stabs.texinfo
INFOFLAGS=	-I${DIST}/gdb/doc -I${DIST}/gdb/mi \
		-I${DIST}/readline/doc -I${.CURDIR}/..

.include "../../Makefile.inc"
.include <bsd.info.mk>

LIBOPCODESDIR!= cd ${.CURDIR}/../opcodes   && ${PRINTOBJDIR}
LIBBFDDIR!=     cd ${.CURDIR}/../bfd       && ${PRINTOBJDIR}
LIBIBERTYDIR!=  cd ${.CURDIR}/../libiberty && ${PRINTOBJDIR}
LIBGDBDIR!=	cd ${.CURDIR}/../libgdb    && ${PRINTOBJDIR}

.if ${USE_LIBEDIT} == yes
LIBRL=-ledit
LIBRLDP=${LIBEDIT}
.else
LIBRLDIR!=  cd ${.CURDIR}/../readline && ${PRINTOBJDIR}
LIBRL=      -L${LIBRLDIR} -lreadline
LIBRLDP=    ${LIBRLDIR}/libreadline.a
.endif

LDADD+= -L${LIBGDBDIR} -lgdb
DPADD+= ${LIBGDBDIR}/libgdb.a

# Simulator support
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SIMOBJ!=	cd ${.CURDIR}/../sim/ppc && ${PRINTOBJDIR}
LDADD+=		-L${SIMOBJ} -lsim
DPADD+=		${SIMOBJ}/libsim.a
.elif ${MACHINE_ARCH} == mipseb || ${MACHINE_ARCH} == mipsel
SIMOBJ!=	cd ${.CURDIR}/../sim/mips && ${PRINTOBJDIR}
LDADD+=		-L${SIMOBJ} -lsim
DPADD+=		${SIMOBJ}/libsim.a
.endif

LDADD+= -L${LIBOPCODESDIR} -lopcodes \
	-L${LIBBFDDIR} -lbfd \
	${LIBRL} \
	-ltermcap \
	-L${LIBIBERTYDIR} -liberty

DPADD+=	${LIBOPCODESDIR}/libopcodes.a \
	${LIBBFDDIR}/libbfd.a \
	${LIBRLDP} \
	${LIBTERMCAP} \
	${LIBIBERTYDIR}/libiberty.a

LDADD+=		-lcurses -lintl -lm -lkvm
DPADD+=		${LIBCURSES} ${LIBINTL} ${LIBM} ${LIBKVM}

.if ${SRCS:Mnbsd-thread.c} != ""
LDADD+=                -lpthread_dbg
.endif

.include <bsd.prog.mk>

# Don't run yacc/bison.
.y.c .y.h:

.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/signals ${DIST}/gdb/doc ${.CURDIR}/../arch/${MACHINE_ARCH}
