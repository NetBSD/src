#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:45 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		stdc++

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	5
SHLIB_MINOR=	0

SUBDIR=		include

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc
GNUHOSTDIST=	${DIST}

LIBMATHSRCS=	${G_libmath_la_SOURCES} ${G_libmath_la_LIBADD:.lo=.c}
LIBSUPCXXSRCS=	${G_LIBSUPCXX_SOURCES} ${G_LIBSUPCXX_C_SOURCES}
LIBSTDCXXSRCS=	${G_SRC_SOURCES}

SRCS=		${LIBMATHSRCS} ${LIBSUPCXXSRCS} ${LIBSTDCXXSRCS}
INCS=		c++config.h
INCSDIR=	/usr/include/g++

#		-I/home/mrg/f2/i386-unknown-netbsdelf1.6U/libstdc++-v3/include/i386-unknown-netbsdelf1.6U
#		-I/home/mrg/f2/i386-unknown-netbsdelf1.6U/libstdc++-v3/include

CPPFLAGS+=	-nostdinc++ -I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${DIST}/gcc -I${DIST}/include -I${DIST}/libstdc++-v3/libsupc++ \
		-I${NETBSDSRCDIR}/gnu/usr.bin/gcc3/libiberty

# affects profiling; can't switch it off just for profiling easily.
#CXXFLAGS+=	${G_SECTION_FLAGS}

CONF=		${DIST}/libstdc++-v3/config
CLEANFILES+=    cxa_demangle.c basic_file.cc c++locale.cc
cxa_demangle.c: ${DIST}/libiberty/cp-demangle.c
	rm -f ${.TARGET}
	ln -s $> ${.TARGET}
basic_file.cc: ${CONF}/io/basic_file_stdio.cc
	cp $> ${.TARGET}
c++locale.cc: ${CONF}/locale/generic/c_locale.cc
	cp $> ${.TARGET}

COPTS.concept-inst.cc=	-D_GLIBCPP_CONCEPT_CHECKS -fimplicit-templates
COPTS.strstream.cc=	-I$(GLIBCPP_INCLUDE_DIR)/backward -Wno-deprecated

.include <bsd.lib.mk>
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
