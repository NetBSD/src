#	$NetBSD: Makefile,v 1.6 2017/11/28 18:09:42 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>
.include "../Makefile.inc"

PROG=	psim
SRCS=	main.c

HOST_CPPFLAGS+=	-I. \
		-I${DIST}/sim/ppc -I${DIST}/include \
		-I${.CURDIR}/../../lib/libsim/arch/${MACHINE_ARCH} \
		-I${.CURDIR}/../../lib/libbfd/arch/${MACHINE_ARCH} \
		-I${DIST}/bfd \
		-I${.CURDIR}/../../lib/libgdb \
		-I${DIST}/gdb \
		-I${DIST}/gdb/config \
		-I${DIST}/sim/common \
		-I${LIBSIMDIR}
PSIM_DEFINES=	-DHAVE_CONFIG_H \
		-DHAVE_COMMON_FPU \
		-DDEFAULT_INLINE=PSIM_INLINE_LOCALS \
		-DWITH_HOST_BYTE_ORDER=BYTE_ORDER \
		-DWITH_SMP=5 \
		-DHAVE_TERMIOS_STRUCTURE \
		-DHAVE_DEVZERO
CPPFLAGS+=	${HOST_CPPFLAGS} ${PSIM_DEFINES}

SRCS+=version.c

.include <bsd.prog.mk>

LIBSIMDIR!=	cd ${.CURDIR}/../../lib/libsim && ${PRINTOBJDIR}

LDADD+=		-L${LIBSIMDIR} -lsim
DPADD+=		${LIBSIMDIR}/libsim.a

LIBBFDDIR!=	cd ${.CURDIR}/../../lib/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${LIBBFDDIR} -lbfd
DPADD+=		${LIBBFDDIR}/libbfd.a

LIBIBERTYDIR!=	cd ${.CURDIR}/../../lib/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${LIBIBERTYDIR} -liberty
DPADD+=		${LIBIBERTYDIR}/libiberty.a

LDADD+=		-lintl -lz
DPADD+=		${LIBINTL} ${LIBZ}

CLEANFILES+=	version.c

version.c: ${DIST}/gdb/version.in
	echo 'const char version[] = "'$$(cat ${.ALLSRC})'";' > ${.TARGET}

.PATH: ${DIST}/sim/ppc 
