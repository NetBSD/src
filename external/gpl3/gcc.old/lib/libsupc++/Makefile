#	$NetBSD: Makefile,v 1.5.4.2 2020/04/13 08:01:56 martin Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

CWARNFLAGS.clang+=	-Wno-parentheses
UNSUPPORTED_COMPILER.clang=	# defined

.include <bsd.init.mk>

GCC_MACHINE_ARCH?=${MACHINE_ARCH:S/earmv5/earm/}

LIB=		supc++

.if exists(${.CURDIR}/../libstdc++-v3/arch/${GCC_MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/../libstdc++-v3/arch/${GCC_MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	2
SHLIB_MINOR=	0

.include "${.CURDIR}/../libsupc++/Makefile.common"

SRCS=		${LIBSUPCXXSRCS} xmalloc.c
CXXFLAGS+=	${G_SECTION_FLAGS}

COPTS.cp-demangle.c = -Wno-stack-protector -Wno-unused-function

.include "../Makefile.gthr"

.include <bsd.lib.mk>

.PATH:	${DIST}/libstdc++-v3/src \
	${DIST}/libstdc++-v3/libsupc++ \
	${DIST}/libiberty \
	${.CURDIR}/../libstdc++-v3/arch/${GCC_MACHINE_ARCH}

${OBJS}: ${.CURDIR}/../libstdc++-v3/arch/${GCC_MACHINE_ARCH}/defs.mk
.else
.include <bsd.prog.mk> # do nothing
.endif
