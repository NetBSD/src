#	$NetBSD: Makefile,v 1.5 2014/01/15 21:14:55 joerg Exp $

LIB=		c++
WARNS=		4
NOLINT=		# defined

.include <bsd.init.mk>

.PATH: ${LIBCXX_SRCDIR}/src

SRCS=	algorithm.cpp bind.cpp chrono.cpp condition_variable.cpp debug.cpp \
	exception.cpp future.cpp hash.cpp ios.cpp iostream.cpp locale.cpp \
	memory.cpp mutex.cpp new.cpp optional.cpp random.cpp regex.cpp \
	shared_mutex.cpp stdexcept.cpp \
	string.cpp strstream.cpp system_error.cpp thread.cpp \
	utility.cpp valarray.cpp
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

CPPFLAGS+=	-nostdinc++ -cxx-isystem ${LIBCXX_SRCDIR}/include -I${LIBCXXRT_SRCDIR}/src
CPPFLAGS+=	-DLIBCXXRT
CXXFLAGS+=	${${ACTIVE_CC} == "clang":? -std=c++11 : -std=c++0x}

CWARNFLAGS.gcc+=	-Wno-error

CWARNFLAGS.clang+=	-Wno-error=unused-variable -Wno-error=missing-noreturn
CWARNFLAGS.clang+=	-Wno-error=missing-prototypes -Wno-error=sometimes-uninitialized
CWARNFLAGS.clang+=	-Wno-error=missing-field-initializers -Wno-error=switch
CWARNFLAGS.clang+=	-Wno-error=implicit-exception-spec-mismatch

LDADD+=	-Wl,-z,defs

.if !empty(MACHINE_ARCH:M*arm*)
# Missing __sync_fetch_and_add_4
# Missing __sync_lock_test_and_set_4
# Missing __sync_val_compare_and_swap_4
# Missing __atomic_fetch_add_4
COPTS.ios.cpp+= -march=armv6
COPTS.locale.cpp+= -march=armv6
COPTS.memory.cpp+= -march=armv6
COPTS.new.cpp+= -march=armv6
COPTS.rt_exception.cc+= -march=armv6
COPTS.rt_guard.cc+= -march=armv6
COPTS.stdexcept.cpp+= -march=armv6
.endif

.include <bsd.lib.mk>
