#	$NetBSD: Makefile,v 1.18 2018/02/03 19:27:15 mrg Exp $

# build GCC's libgomp, so that -fopenmp works.

# XXX
NOLINT=	# defined

.include <bsd.own.mk>

.if !defined(NO_LIBGOMP)	# {

DIST=	${GCCDIST}

LIB=	gomp
SRCS= \
	alloc.c \
	barrier.c \
	critical.c \
	env.c \
	error.c \
	iter.c \
	iter_ull.c \
	loop.c \
	loop_ull.c \
	ordered.c \
	parallel.c \
	sections.c \
	single.c \
	task.c \
	team.c \
	work.c \
	lock.c \
	mutex.c \
	proc.c \
	sem.c \
	bar.c \
	ptrlock.c \
	time.c \
	fortran.c \
	affinity.c \
	target.c \
	splay-tree.c \
	libgomp-plugin.c \
	oacc-parallel.c \
	oacc-host.c \
	oacc-init.c \
	oacc-mem.c \
	oacc-async.c \
	oacc-plugin.c \
	oacc-cuda.c \
	priority_queue.c


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

CWARNFLAGS.clang+=	-Wno-conversion

# libgomp is 1.0 is in GCC 4.5, and 1.1 in 4.8 since it added more symbols
# XXXGCC5 -- marking 1.2 for GCC 5.3 for now
SHLIB_MAJOR=	1
SHLIB_MINOR=	2

LDFLAGS+=	-Wl,--version-script,${DIST}/libgomp/libgomp.map

.if ${GOMP_MACHINE_ARCH} == ${MACHINE_ARCH:S/earmv5/earm/}
# This is where GCC looks for it.
FILES=		libgomp.spec
FILESDIR=	${LIBDIR}

INCS=		omp.h openacc.h
INCSDIR=	/usr/include/gcc-6
.endif

.else			# } else {
libinstall::
.endif			# }

.include <bsd.lib.mk>

COPTS+=	-Wno-stack-protector -Wno-missing-prototypes
COPTS.oacc-mem.c+= -Wno-pointer-arith

.PATH:	${ARCHDIR} ${DIST}/libgomp ${DIST}/libgomp/config/posix
