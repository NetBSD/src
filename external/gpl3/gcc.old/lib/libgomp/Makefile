#	$NetBSD: Makefile,v 1.12 2020/08/19 07:47:43 mrg Exp $

# build GCC's libgomp, so that -fopenmp works.

# XXX
NOLINT=	# defined

UNSUPPORTED_COMPILER.clang=	# defined

.include <bsd.own.mk>

.if !defined(NO_LIBGOMP)	# {

DIST=	${GCCDIST}


LIB=	gomp

.include "${.CURDIR}/defs.mk"
SRCS=	${G_libgomp_la_SOURCES}

GOMP_MACHINE_ARCH?=	${MACHINE_ARCH:S/earmv5/earm/}
ARCHDIR=		${.CURDIR}/arch/${GOMP_MACHINE_ARCH}

CPPFLAGS+= \
	-DHAVE_CONFIG_H \
	-I${DIST}/libgomp \
	-I${DIST}/libgomp/config/posix \
	-I${DIST}/libgcc \
	-I${DIST}/include \
	-I${ARCHDIR}
CFLAGS+=-pthread

SHLIB_MAJOR=	2
SHLIB_MINOR=	0

LDFLAGS+=	-Wl,--version-script,${DIST}/libgomp/libgomp.map

.if ${GOMP_MACHINE_ARCH} == ${MACHINE_ARCH:S/earmv5/earm/}
# This is where GCC looks for it.
FILES=		libgomp.spec
FILESDIR=	${LIBDIR}

INCS=		omp.h openacc.h
INCSDIR=	${GCC_INCSDIR}
.endif

.else			# } else {
libinstall::
.endif			# }

.include <bsd.lib.mk>

COPTS+=	-Wno-stack-protector -Wno-missing-prototypes
COPTS.oacc-mem.c+= -Wno-pointer-arith

COPTS.lock.c+=		-Wno-error=incompatible-pointer-types
COPTS.target.c+=	-Wno-error=pointer-arith

.PATH: ${DIST}/libgomp/config/posix
.PATH: ${ARCHDIR} ${DIST}/libgomp
