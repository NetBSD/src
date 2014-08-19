#	$NetBSD: Makefile,v 1.1.1.2.8.2 2014/08/19 23:58:23 tls Exp $

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

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

.include "${.CURDIR}/../libsupc++/Makefile.common"

SRCS=		${LIBSUPCXXSRCS} xmalloc.c
CXXFLAGS+=	${G_SECTION_FLAGS}

COPTS.cp-demangle.c = -Wno-stack-protector

.include <bsd.lib.mk>

${SRCS}: unwind.h
unwind.h: ${G_UNWIND_H}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${G_UNWIND_H} ${.TARGET}
CLEANFILES+= unwind.h

.PATH:	${DIST}/libstdc++-v3/src \
	${DIST}/libstdc++-v3/libsupc++ \
	${DIST}/libiberty \
	${.CURDIR}/../libstdc++-v3/arch/${GCC_MACHINE_ARCH}

${OBJS}: ${.CURDIR}/../libstdc++-v3/arch/${GCC_MACHINE_ARCH}/defs.mk
.else
.include <bsd.prog.mk> # do nothing
.endif
