#	$NetBSD: Makefile,v 1.1 2006/05/10 22:53:50 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		stdc++

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	6
SHLIB_MINOR=	0

SUBDIR=		include

INCS=		c++config.h
INCSDIR=	/usr/include/g++

.include "${.CURDIR}/../libsupc++4/Makefile.common"

SRCS=		${LIBMATHSRCS} ${LIBSUPCXXSRCS} ${LIBSTDCXXSRCS}
# affects profiling; can't switch it off just for profiling easily.
#CXXFLAGS+=	${G_SECTION_FLAGS}
#CXXFLAGS+=	-ffunction-sections -fdata-sections 
CXXFLAGS+=	-fno-implicit-templates -fdiagnostics-show-location=once
CPPFLAGS+=	-I${DIST}/libstdc++-v3/include

CONF=		${DIST}/libstdc++-v3/config

#BUILDSYMLINKS+=	${CONF}/io/basic_file_stdio.cc		basic_file.cc \
#		${CONF}/locale/generic/c_locale.cc	c++locale.cc

GLIBCPP_INCLUDE_DIR=	${DESTDIR}/usr/include/g++

COPTS.concept-inst.cc=	-D_GLIBCPP_CONCEPT_CHECKS -fimplicit-templates
CPPFLAGS.strstream.cc=	-I$(GLIBCPP_INCLUDE_DIR)/backward -Wno-deprecated

# XXX
MKDEPFLAGS+=	-I$(GLIBCPP_INCLUDE_DIR)/backward

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
.include <bsd.info.mk>

GLIBCXX_SRCDIR=	${DIST}/libstdc++-v3

codecvt_members.cc: ${GLIBCXX_SRCDIR}/${G_CCODECVT_CC}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${GLIBCXX_SRCDIR}/${G_CCODECVT_CC} ${.TARGET}

collate_members.cc: ${GLIBCXX_SRCDIR}/${G_CCOLLATE_CC}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${GLIBCXX_SRCDIR}/${G_CCOLLATE_CC} ${.TARGET}

ctype_members.cc: ${GLIBCXX_SRCDIR}/${G_CCTYPE_CC}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${GLIBCXX_SRCDIR}/${G_CCTYPE_CC} ${.TARGET}

messages_members.cc: ${GLIBCXX_SRCDIR}/${G_CMESSAGES_CC}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${GLIBCXX_SRCDIR}/${G_CMESSAGES_CC} ${.TARGET}

monetary_members.cc: ${GLIBCXX_SRCDIR}/${G_CMONEY_CC}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${GLIBCXX_SRCDIR}/${G_CMONEY_CC} ${.TARGET}

numeric_members.cc: ${GLIBCXX_SRCDIR}/${G_CNUMERIC_CC}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${GLIBCXX_SRCDIR}/${G_CNUMERIC_CC} ${.TARGET}

time_members.cc: ${GLIBCXX_SRCDIR}/${G_CTIME_CC}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${GLIBCXX_SRCDIR}/${G_CTIME_CC} ${.TARGET}

atomicity.cc: ${G_atomicity_file}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${G_atomicity_file} ${.TARGET}

c++locale.cc: ${GLIBCXX_SRCDIR}/${G_CLOCALE_CC}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${GLIBCXX_SRCDIR}/${G_CLOCALE_CC} ${.TARGET}

basic_file.cc: ${GLIBCXX_SRCDIR}/${G_BASIC_FILE_CC}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${GLIBCXX_SRCDIR}/${G_BASIC_FILE_CC} ${.TARGET}

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
