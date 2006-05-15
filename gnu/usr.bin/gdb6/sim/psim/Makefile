#	$NetBSD: Makefile,v 1.2 2006/05/15 19:11:51 nathanw Exp $

NOMAN=	# defined

.include <bsd.own.mk>

TOP=	${NETBSDSRCDIR}/gnu
DIST=	${TOP}/dist/gdb6

PROG=	psim
SRCS=	main.c

HOST_CPPFLAGS+=	-I. \
		-I${DIST}/sim/ppc -I${DIST}/include \
		-I${.CURDIR}/../../arch/powerpc \
		-I${DIST}/bfd \
		-I${.CURDIR}/../../gdb \
		-I${DIST}/gdb \
		-I${DIST}/gdb/config \
		-DHAVE_COMMON_FPU \
		-I${.CURDIR}/../arch/powerpc \
		-I${DIST}/sim/common
PSIM_DEFINES=	-DHAVE_CONFIG_H \
		-DDEFAULT_INLINE=PSIM_INLINE_LOCALS \
		-DWITH_HOST_BYTE_ORDER=BYTE_ORDER \
		-DWITH_SMP=5 \
		-DHAVE_TERMIOS_STRUCTURE \
		-DHAVE_DEVZERO
CPPFLAGS+=	${HOST_CPPFLAGS} ${PSIM_DEFINES}

.include "../../../Makefile.inc"
.include <bsd.prog.mk>

SIMDIR!=	cd ${.CURDIR}/../ppc && ${PRINTOBJDIR}

LDADD+=		-L${SIMDIR} -lsim
DPADD+=		${SIMDIR}/libsim.a

LIBBFDDIR!=	cd ${.CURDIR}/../../bfd && ${PRINTOBJDIR}
LDADD+=		-L${LIBBFDDIR} -lbfd
DPADD+=		${LIBBFDDIR}/libbfd.a

LIBIBERTYDIR!=	cd ${.CURDIR}/../../libiberty && ${PRINTOBJDIR}
LDADD+=		-L${LIBIBERTYDIR} -liberty
DPADD+=		${LIBIBERTYDIR}/libiberty.a

.PATH: ${DIST}/sim/ppc 
