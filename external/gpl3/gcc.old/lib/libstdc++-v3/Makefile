#	$NetBSD: Makefile,v 1.8 2017/10/05 23:46:34 christos Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined
UNSUPPORTED_COMPILER.clang=	# defined

.include <bsd.init.mk>

.include "Makefile.inc"

LIB=		stdc++

LIBDPLIBS+=	m ${.CURDIR}/../../../../../lib/libm

#COPTS+=		-pthread
#LDFLAGS+=	-pthread
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
COPTS.random.cc+=	${${ACTIVE_CC} == "clang":? -mrdrnd :}
.endif

CXXFLAGS.clang+=	-stdlib=libstdc++ -std=c++11 -D_GLIBCXX_ABI_TAG_CXX11=
CXXFLAGS+=	${CXXFLAGS.${ACTIVE_CC}}
CWARNFLAGS.clang+=	-Wno-logical-op-parentheses \
			-Wno-deprecated-writable-strings -Wno-parentheses

.include "${.CURDIR}/../libsupc++/Makefile.common"

.if exists(${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	8
SHLIB_MINOR=	0

SUBDIR=		include

SRCS=		${LIBSUPCXXSRCS} ${LIBSTDCXXSRCS}
# affects profiling; can't switch it off just for profiling easily.
#CXXFLAGS+=	${G_SECTION_FLAGS}
#CXXFLAGS+=	-ffunction-sections -fdata-sections 
CXXFLAGS+=	-fno-implicit-templates -fdiagnostics-show-location=once
CPPFLAGS+=	-I${DIST}/libstdc++-v3/include
CPPFLAGS+=	-I${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}
# include backwards compatible code
CPPFLAGS+=     -D_GLIBCXX_SHARED -DGTHREAD_USE_WEAK -DSUPPORTS_WEAK

CONF=		${DIST}/libstdc++-v3/config

GLIBCPP_INCLUDE_DIR=	${DESTDIR}/usr/include/g++

# Template instantiation bug workaround for -O<2/s
COPTS.cp-demangle.c += -Wno-stack-protector
COPTS.cp-demangle.c += -Wno-stack-protector
COPTS.fstream-inst.cc += -Wno-stack-protector
COPTS.locale-inst.cc += -Wno-stack-protector
COPTS.misc-inst.cc += -Wno-stack-protector
COPTS.ostream-inst.cc += -Wno-stack-protector
COPTS.functexcept.cc += -Wno-stack-protector
COPTS.valarray-inst.cc += -Wno-stack-protector
COPTS.wlocale-inst.cc += -Wno-stack-protector
COPTS.snprintf_lite.cc += -Wno-stack-protector
# From system headers.
COPTS.cxx11-wlocale-inst.cc +=-Wno-stack-protector
COPTS.cxx11-locale-inst.cc +=-Wno-stack-protector
COPTS.cxx11-wlocale-inst.cc +=-Wno-stack-protector
COPTS.compatibility-thread-c++0x.cc += -Wno-deprecated-declarations
COPTS.concept-inst.cc += -fimplicit-templates
COPTS.hashtable_c++0x.cc += -fimplicit-templates
COPTS.bitmap_allocator.cc += -fimplicit-templates
COPTS.functexcept.cc += -Wno-deprecated-declarations
COPTS.future.cc += -Wno-deprecated-declarations
COPTS.shared_ptr.cc += -Wno-deprecated-declarations
COPTS.thread.cc += -Wno-deprecated-declarations
CPPFLAGS.strstream.cc += -I$(GLIBCPP_INCLUDE_DIR)/backward -Wno-deprecated

CPPFLAGS.cp-demangle.cc += -DIN_GLIBCPP_V3
CPPFLAGS.concept-inst.cc += -D_GLIBCXX_CONCEPT_CHECKS
CPPFLAGS.parallel_list.cc += -D_GLIBCXX_PARALLEL
CPPFLAGS.parallel_settings.cc += -D_GLIBCXX_PARALLEL
CPPFLAGS.compatibility-parallel_list.cc += -D_GLIBCXX_PARALLEL

cp-demangle.d cp-demangle.o: cp-demangle.h

.if ${MACHINE_ARCH} == "i386"
COPTS.random.cc += -Wno-error=psabi
COPTS.cow-string-inst.cc += -Wno-error=psabi
.endif

# XXX
MKDEPFLAGS+=	-I$(GLIBCPP_INCLUDE_DIR)/backward

GLIBCXX_SRCDIR=	${DIST}/libstdc++-v3

BUILDSYMLINKS+=	\
	${GLIBCXX_SRCDIR}/${G_CCODECVT_CC} codecvt_members.cc \
	${GLIBCXX_SRCDIR}/${G_CCOLLATE_CC} collate_members.cc \
	${GLIBCXX_SRCDIR}/${G_CCTYPE_CC} ctype_members.cc \
	${GLIBCXX_SRCDIR}/${G_CMESSAGES_CC} messages_members.cc \
	${GLIBCXX_SRCDIR}/${G_CMONEY_CC} monetary_members.cc \
	${GLIBCXX_SRCDIR}/${G_CNUMERIC_CC} numeric_members.cc \
	${GLIBCXX_SRCDIR}/${G_CTIME_CC} time_members.cc \
	${GLIBCXX_SRCDIR}/${G_ATOMICITY_SRCDIR}/atomicity.h atomicity.cc \
	${GLIBCXX_SRCDIR}/${G_CLOCALE_CC} c++locale.cc \
	${GLIBCXX_SRCDIR}/${G_BASIC_FILE_CC} basic_file.cc \
	${DIST}/libiberty/cp-demangle.c cp-demangle.c \
	${DIST}/libiberty/cp-demangle.h cp-demangle.h \
	${GLIBCXX_SRCDIR}/${G_CCOLLATE_CC} collate_members_cow.cc \
	${GLIBCXX_SRCDIR}/${G_CMESSAGES_CC} messages_members_cow.cc \
	${GLIBCXX_SRCDIR}/${G_CMONEY_CC} monetary_members_cow.cc \
	${GLIBCXX_SRCDIR}/${G_CNUMERIC_CC} numeric_members_cow.cc

# XXX both c++98 and c++11 have codecvt.cc files.
BUILDSYMLINKS+=	\
	${GLIBCXX_SRCDIR}/src/c++98/codecvt.cc c98-codecvt.cc
BUILDSYMLINKS+=	\
	${GLIBCXX_SRCDIR}/src/c++11/codecvt.cc c11-codecvt.cc
SRCS+=	c98-codecvt.cc c11-codecvt.cc

.if ${HAVE_LIBGCC_EH} == "no"
G_CONFIGLINKS:= ${G_CONFIGLINKS:S,libgcc/config/arm/unwind-arm.h,libgcc/unwind-generic.h,}
.endif

.if ${MKGCC} != "no"
.if defined(G_CONFIGLINKS) && !empty(G_CONFIGLINKS)
BUILDSYMLINKS+=        ${G_CONFIGLINKS}
.for _src _dst in ${G_CONFIGLINKS}
DPSRCS+=       ${_dst}
.endfor
.endif
.endif

# XXX Make this either/or,  not one, and maybe another
.for _s in ${SRCS:M*.cc}
COPTS.${_s}+= -std=gnu++11
.endfor
.for _s in ${G_CPP98_SOURCES:Nlocalename.cc:Nlocale_init.cc:M*.cc}
COPTS.${_s}+= -std=gnu++98
.endfor

COPTS.del_ops.cc += -Wno-c++14-compat
COPTS.del_opvs.cc += -Wno-c++14-compat
COPTS.cp-demangle.c += -Wno-unused-function

.for f in collate_members_cow \
	  messages_members_cow \
          monetary_members_cow \
	  numeric_members_cow
COPTS.${f}.cc+= -fimplicit-templates ${G_GLIBCXX_ABI_FLAGS}
.endfor

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
.include <bsd.info.mk>

.PATH:	${DIST}/libstdc++-v3/src \
	${DIST}/libstdc++-v3/src/c++98 \
	${DIST}/libstdc++-v3/src/c++11 \
	${DIST}/libstdc++-v3/libsupc++ \
	${DIST}/libstdc++-v3/config/os/bsd/netbsd \
	${CONF}/locale/generic \
	${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}

${OBJS}: ${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}/defs.mk
.else
.include <bsd.prog.mk> # do nothing
.endif
