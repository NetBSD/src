#	$NetBSD: Makefile,v 1.2 2003/07/26 14:55:14 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		stdc++

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	5
SHLIB_MINOR=	0

SUBDIR=		include

INCS=		c++config.h
INCSDIR=	/usr/include/g++

.include "${.CURDIR}/../libsupc++/Makefile.common"

SRCS=		${LIBMATHSRCS} ${LIBSUPCXXSRCS} ${LIBSTDCXXSRCS} xmalloc.c
# affects profiling; can't switch it off just for profiling easily.
#CXXFLAGS+=	${G_SECTION_FLAGS}

CONF=		${DIST}/libstdc++-v3/config
CLEANFILES+=    basic_file.cc c++locale.cc
basic_file.cc: ${CONF}/io/basic_file_stdio.cc
	cp $> ${.TARGET}
c++locale.cc: ${CONF}/locale/generic/c_locale.cc
	cp $> ${.TARGET}

COPTS.concept-inst.cc=	-D_GLIBCPP_CONCEPT_CHECKS -fimplicit-templates
COPTS.strstream.cc=	-I$(GLIBCPP_INCLUDE_DIR)/backward -Wno-deprecated

# ARGH!
#.include <bsd.lib.mk>
.include <bsd.subdir.mk>
.include <bsd.info.mk>

.PATH:	${DIST}/libstdc++-v3/src \
	${DIST}/libstdc++-v3/libmath \
	${DIST}/libstdc++-v3/libsupc++ \
	${DIST}/libiberty \
	${CONF}/locale/generic \
	${.CURDIR}/arch/${MACHINE_ARCH}

${OBJS}: ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk
.else
.include <bsd.prog.mk> # do nothing
.endif

.if (${OBJECT_FMT} == "a.out")
LDADD=	${DESTDIR}/usr/lib/c++rt0.o
DPADD=	${DESTDIR}/usr/lib/c++rt0.o
.endif
