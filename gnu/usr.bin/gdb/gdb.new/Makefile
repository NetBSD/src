#	$NetBSD: Makefile,v 1.9 2002/02/11 21:45:20 tv Exp $

# XXX - This gdb.new directory should be renamed to "gdb" after all platforms
# have been migrated to the new toolchain.

.include <bsd.own.mk>

.if ${MKGDB} != "no" && ${MKBFD} != "no" && \
    exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

PROG=		gdb

SRCS=		${G_LIBGDB_OBS:.o=.c} ${G_OBJECTS:Nxmalloc.o:.o=.c} ${G_SIM_OBS:.o=.c} main.c
CPPFLAGS=	-I${.CURDIR}/../arch/${MACHINE_ARCH} \
		${G_INTERNAL_CFLAGS:M-[ID]*:N*readline*:N*intl*:N-I.*} \
		${G_CCFLAGS:M-DRL_*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH}

LDADD=		-ltermcap -lintl -lm -lkvm
DPADD=		${LIBTERMCAP} ${LIBINTL} ${LIBM} ${LIBKVM}

TEXINFO=	gdb.texinfo gdbint.texinfo stabs.texinfo
INFOFLAGS=	-I${DIST}/gdb/doc -I${DIST}/gdb/mi \
		-I${DIST}/readline/doc -I${.CURDIR}/..

.endif	# MKGDB != no && MKBFD != no

.include "../../Makefile.inc"
.include <bsd.prog.mk>
.include <bsd.info.mk>

.if ${MKGDB} != "no" && ${MKBFD} != "no" && \
    exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)

BFDOBJ!=	cd ${TOP}/lib/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd_pic.a

IBERTYOBJ!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

# PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
PSIMOBJ!=	cd ${TOP}/usr.bin/sim/ppc && ${PRINTOBJDIR}
LDADD+=		-L${PSIMOBJ} -lsim
DPADD+=		${PSIMOBJ}/libsim.a
.endif

.PATH: ${DIST}/gdb ${DIST}/gdb/tui ${DIST}/gdb/doc \
	${DIST}/readline ${.CURDIR}/../arch/${MACHINE_ARCH}

.endif	# MKGDB != no && MKBFD != no
