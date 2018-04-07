# $NetBSD: Makefile,v 1.21.2.1 2018/04/07 04:12:08 pgoyette Exp $

UNSUPPORTED_COMPILER.clang=	# defined

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/asan

ASAN_SRCS= \
asan_activation.cc \
asan_allocator.cc \
asan_debugging.cc \
asan_flags.cc \
asan_fake_stack.cc \
asan_globals.cc \
asan_interceptors.cc \
asan_linux.cc \
asan_mac.cc \
asan_malloc_linux.cc \
asan_malloc_mac.cc \
asan_malloc_win.cc \
asan_new_delete.cc \
asan_poisoning.cc \
asan_posix.cc \
asan_report.cc \
asan_rtl.cc \
asan_stack.cc \
asan_stats.cc \
asan_suppressions.cc \
asan_thread.cc \
asan_win.cc \
asan_win_dll_thunk.cc \
asan_win_dynamic_runtime_thunk.cc

LIB=	asan
SRCS+=	${ASAN_SRCS}
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt

.if ${MACHINE_ARCH} == "vax"
COPTS.asan_allocator.cc += -O1
COPTS.asan_report.cc += -O1
COPTS.sanitizer_common.cc += -O1
COPTS.sanitizer_common_libcdep.cc += -O1
COPTS.sanitizer_posix.cc += -O1
COPTS.ubsan_diag.cc += -O1
COPTS.ubsan_init.cc += -O1
.endif

.include <bsd.lib.mk>
