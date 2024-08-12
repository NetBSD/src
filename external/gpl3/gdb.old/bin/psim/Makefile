#	$NetBSD: Makefile,v 1.10 2024/08/12 20:30:05 christos Exp $

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

# /home/source/ab/HEAD-llvm/src/external/gpl3/gdb/bin/psim/../../dist/include/sim/sim.h:37:27: error: redefinition of typedef 'SIM_DESC' is a C11 feature [-Werror,-Wtypedef-redefinition]
# typedef struct sim_state *SIM_DESC;
#                           ^
# /home/source/ab/HEAD-llvm/src/external/gpl3/gdb/bin/psim/../../dist/sim/ppc/sim_callbacks.h:106:27: note: previous definition is here
# typedef struct sim_state *SIM_DESC;
#                           ^
#
# XXX remove me when we use -std=c11 or -std=gnu11 or something
CWARNFLAGS.clang+=	-Wno-error=typedef-redefinition

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
