#	$NetBSD: Makefile,v 1.18 2003/03/01 13:09:27 mrg Exp $

.include <bsd.own.mk>

.if ${MKGDB} != "no" && ${MKBFD} != "no" && \
    exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

PROG=		gdb

SRCS=		${G_LIBGDB_OBS:.o=.c} ${G_OBJECTS:Nxmalloc.o:.o=.c} ${G_SIM_OBS:.o=.c} main.c
CPPFLAGS=	-I${.CURDIR}/../arch/${MACHINE_ARCH} \
		${G_INTERNAL_CFLAGS:M-[ID]*:N*readline*:N*intl*:N-I.*} \
		${G_CCFLAGS:M-DRL_*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH}

LDADD=		-ltermcap -lintl -lm -lkvm
DPADD=		${LIBTERMCAP} ${LIBINTL} ${LIBM} ${LIBKVM}

.if ${SRCS:Mnbsd-thread.c} != ""
LDADD+=                -lpthread_dbg
.endif

TEXINFO=	gdb.texinfo gdbint.texinfo stabs.texinfo
INFOFLAGS=	-I${DIST}/gdb/doc -I${DIST}/gdb/mi \
		-I${DIST}/readline/doc -I${.CURDIR}/..

.endif	# MKGDB != no && MKBFD != no

.include "../../Makefile.inc"
.include <bsd.prog.mk>
.include <bsd.info.mk>

.if ${MKGDB} != "no" && ${MKBFD} != "no" && \
    exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)

# PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
PSIMOBJ!=	cd ${TOP}/usr.bin/sim/ppc && ${PRINTOBJDIR}
LDADD+=		-L${PSIMOBJ} -lsim
DPADD+=		${PSIMOBJ}/libsim.a
.endif

OPCODESOBJ!=	cd ${TOP}/lib/libopcodes && ${PRINTOBJDIR}
LDADD+=		-L${OPCODESOBJ} -lopcodes
DPADD+=		${OPCODESOBJ}/libopcodes_pic.a

BFDOBJ!=	cd ${TOP}/lib/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd_pic.a

IBERTYOBJ!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

.PATH: ${DIST}/gdb ${DIST}/gdb/tui ${DIST}/gdb/doc \
	${DIST}/readline ${.CURDIR}/../arch/${MACHINE_ARCH}

.endif	# MKGDB != no && MKBFD != no
