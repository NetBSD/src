#	$NetBSD: Makefile,v 1.14 2021/05/30 01:57:00 joerg Exp $

LIB=		c++
WARNS=		4
NOLINT=		# defined

.include <bsd.init.mk>

.PATH: ${LIBCXX_SRCDIR}/src

SRCS=	algorithm.cpp bind.cpp charconv.cpp chrono.cpp condition_variable.cpp \
	condition_variable_destructor.cpp debug.cpp exception.cpp format.cpp \
	functional.cpp future.cpp hash.cpp ios.cpp ios.instantiations.cpp \
	iostream.cpp locale.cpp memory.cpp mutex.cpp mutex_destructor.cpp \
	new.cpp optional.cpp random.cpp random_shuffle.cpp regex.cpp \
	shared_mutex.cpp stdexcept.cpp string.cpp strstream.cpp \
	system_error.cpp thread.cpp utility.cpp valarray.cpp variant.cpp \
	vector.cpp

.PATH: ${LIBCXX_SRCDIR}/src/filesystem
SRCS+=	directory_iterator.cpp operations.cpp

# typeinfo.cc: prefer libcxxrt's version

LIBCXXRT_SRCS+= \
	auxhelper.cc dynamic_cast.cc exception.cc guard.cc \
	stdexcept.cc terminate.cc typeinfo.cc libelftc_dem_gnu3.c
# cxa_atexit.c: Solaris-only
# memory.cc: already provided by libc++'s new.cpp

.for src in ${LIBCXXRT_SRCS}
rt_${src}: ${LIBCXXRT_SRCDIR}/src/${src}
	cp ${LIBCXXRT_SRCDIR}/src/${src} rt_${src}

SRCS+=		rt_${src}
DPSRCS+=	rt_${src}
CLEANFILES+=	rt_${src}
.endfor

.include <bsd.own.mk>

CPPFLAGS+=	-nostdinc++ -cxx-isystem ${LIBCXX_SRCDIR}/include
CPPFLAGS+=	-cxx-isystem ${.CURDIR}/../include -I${LIBCXXRT_SRCDIR}/src
CPPFLAGS+=	-DLIBCXXRT -D_LIBCPP_BUILDING_LIBRARY
CXXFLAGS+=	-std=gnu++17

CWARNFLAGS.gcc+=	-Wno-error

CWARNFLAGS.clang+=	-Wno-error=unused-variable -Wno-error=missing-noreturn
CWARNFLAGS.clang+=	-Wno-error=missing-prototypes -Wno-error=sometimes-uninitialized
CWARNFLAGS.clang+=	-Wno-error=missing-field-initializers -Wno-error=switch
CWARNFLAGS.clang+=	-Wno-error=implicit-exception-spec-mismatch -Wno-error=format-nonliteral -Wno-error=format-security

.if ${MKSANITIZER} != "yes"
LDFLAGS+=	-Wl,-z,defs
.endif

.if !empty(MACHINE_ARCH:Marm*) || !empty(MACHINE_ARCH:Mearm*)
CWARNFLAGS+=		-Wno-atomic-alignment
.endif

.include <bsd.lib.mk>
