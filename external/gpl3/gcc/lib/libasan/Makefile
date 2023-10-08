# $NetBSD: Makefile,v 1.33.6.1 2023/10/08 14:35:22 martin Exp $

UNSUPPORTED_COMPILER.clang=	# defined
NOSANITIZER=	# defined

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/asan ${SANITIZER}/lsan

# XXXmknative
ASAN_SRCS= \
	asan_activation.cc \
	asan_allocator.cc \
	asan_debugging.cc \
	asan_descriptions.cc \
	asan_errors.cc \
	asan_fake_stack.cc \
	asan_flags.cc \
	asan_globals.cc \
	asan_interceptors.cc \
	asan_interceptors_memintrinsics.cc \
	asan_linux.cc \
	asan_mac.cc \
	asan_malloc_linux.cc \
	asan_malloc_mac.cc \
	asan_malloc_win.cc \
	asan_memory_profile.cc \
	asan_new_delete.cc \
	asan_poisoning.cc \
	asan_posix.cc \
	asan_report.cc \
	asan_rtl.cc \
	asan_shadow_setup.cc \
	asan_stack.cc \
	asan_stats.cc \
	asan_suppressions.cc \
	asan_thread.cc \
	asan_win.cc \
	asan_win_dll_thunk.cc \
	asan_win_dynamic_runtime_thunk.cc

LIB=	asan
SRCS+=	${ASAN_SRCS} lsan_common.cc lsan_common_linux.cc
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt
LIBDPLIBS+= m  ${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+= pthread ${NETBSDSRCDIR}/lib/libpthread
CPPFLAGS+=-DCAN_SANITIZE_UB=0

.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
COPTS.asan_interceptors.cc += -O1
.endif

.include <bsd.lib.mk>
