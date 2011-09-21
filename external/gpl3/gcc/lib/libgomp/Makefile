#	$NetBSD: Makefile,v 1.2 2011/09/21 02:12:00 mrg Exp $

# build GCC's libgomp, so that -fopenmp works.

.include <bsd.own.mk>
DIST=	${NETBSDSRCDIR}/external/gpl3/gcc/dist

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
	affinity.c

GOMP_MACHINE_ARCH?=	${MACHINE_ARCH}

CPPFLAGS+= \
	-DHAVE_CONFIG_H \
	-I${DIST}/libgomp \
	-I${DIST}/libgomp/config/posix \
	-I${DIST}/libgcc \
	-I${.CURDIR}/arch/${GOMP_MACHINE_ARCH}
CFLAGS+=-pthread

# This is where GCC looks for it.
FILES=		libgomp.spec
FILESDIR=	${LIBDIR}

# libgomp.so.1.0 is in GCC 4.5.
SHLIB_MAJOR=	1
SHLIB_MINOR=	0

LDFLAGS+=	-Wl,--version-script,${DIST}/libgomp/libgomp.map

.PATH:	${DIST}/libgomp ${DIST}/libgomp/config/posix

.include <bsd.lib.mk>
