#	$NetBSD: Makefile,v 1.5 2011/05/26 12:56:27 joerg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

CWARNFLAGS.clang+=	-Wno-parentheses

.include <bsd.own.mk>

LIB=		supc++

.if exists(${.CURDIR}/../libstdc++-v3_4/arch/${MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/../libstdc++-v3_4/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

.include "${.CURDIR}/../libsupc++4/Makefile.common"

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
	${DIST}/libstdc++-v3/libmath \
	${DIST}/libstdc++-v3/libsupc++ \
	${DIST}/libiberty \
	${.CURDIR}/../libstdc++-v3_4/arch/${MACHINE_ARCH}

${OBJS}: ${.CURDIR}/../libstdc++-v3_4/arch/${MACHINE_ARCH}/defs.mk
.else
.include <bsd.prog.mk> # do nothing
.endif
