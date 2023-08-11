#	$NetBSD: Makefile,v 1.3 2023/08/11 15:18:29 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>
.include "../Makefile.inc"

PROG=	psim
SRCS=	main.c

HOST_CPPFLAGS+=	-I. \
		-I${DIST}/sim/ppc -I${DIST}/include \
		-I${.CURDIR}/../../lib/libsim/arch/${MACHINE_ARCH} \
		-I${.CURDIR}/../../lib/libbfd/arch/${MACHINE_ARCH} \
		-I${.CURDIR}/../../lib/libgnulib/arch/${MACHINE_ARCH} \
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
		-DSIM_COMMON_BUILD \
		-DHAVE_TERMIOS_STRUCTURE \
		-DHAVE_DEVZERO \
		-DRETSIGTYPE=void \
		-DWITH_STDIO=0

CPPFLAGS+=	${HOST_CPPFLAGS} ${PSIM_DEFINES}

SRCS+=version.c

GDBLIBDIR=	${.CURDIR}/../../lib
PROGDPLIBS+=	sim ${GDBLIBDIR}/libsim \
		bfd ${GDBLIBDIR}/libbfd \
		sframe ${GDBLIBDIR}/libsframe \
		iberty ${GDBLIBDIR}/libiberty \

.include <bsd.prog.mk>


LDADD+=		-lintl -lz
DPADD+=		${LIBINTL} ${LIBZ}

CLEANFILES+=	version.c

version.c: ${DIST}/gdb/version.in
	echo 'const char version[] = "'$$(cat ${.ALLSRC})'";' > ${.TARGET}

.PATH: ${DIST}/sim/ppc 
