#	$NetBSD: Makefile,v 1.2.28.1 2013/12/18 18:41:40 matt Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

LIB=		supc++

.include "${.CURDIR}/../Makefile.inc"

.if exists(${.CURDIR}/../libstdc++-v3_4/arch/${GCC_MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/../libstdc++-v3_4/arch/${GCC_MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	0
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
	${.CURDIR}/../libstdc++-v3_4/arch/${GCC_MACHINE_ARCH}

${OBJS}: ${.CURDIR}/../libstdc++-v3_4/arch/${GCC_MACHINE_ARCH}/defs.mk
.else
.include <bsd.prog.mk> # do nothing
.endif

.if (${OBJECT_FMT} == "a.out")
LDADD=	${DESTDIR}/usr/lib/c++rt0.o
DPADD=	${DESTDIR}/usr/lib/c++rt0.o
.endif
