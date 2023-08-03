# $NetBSD: Makefile,v 1.34 2023/08/03 01:36:55 mrg Exp $

UNSUPPORTED_COMPILER.clang=	# defined
NOSANITIZER=	# defined

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/asan ${SANITIZER}/lsan

# XXXmknative
ASAN_SRCS= \
	asan_activation.cpp \
	asan_allocator.cpp \
	asan_debugging.cpp \
	asan_descriptions.cpp \
	asan_errors.cpp \
	asan_fake_stack.cpp \
	asan_flags.cpp \
	asan_globals.cpp \
	asan_interceptors.cpp \
	asan_interceptors_memintrinsics.cpp \
	asan_linux.cpp \
	asan_mac.cpp \
	asan_malloc_linux.cpp \
	asan_malloc_mac.cpp \
	asan_malloc_win.cpp \
	asan_memory_profile.cpp \
	asan_new_delete.cpp \
	asan_poisoning.cpp \
	asan_posix.cpp \
	asan_premap_shadow.cpp \
	asan_report.cpp \
	asan_rtl.cpp \
	asan_shadow_setup.cpp \
	asan_stack.cpp \
	asan_stats.cpp \
	asan_suppressions.cpp \
	asan_thread.cpp \
	asan_win.cpp \
	asan_win_dll_thunk.cpp \
	asan_win_dynamic_runtime_thunk.cpp \
	asan_interceptors_vfork.S

LIB=	asan
SRCS+=	${ASAN_SRCS} lsan_common.cpp lsan_common_linux.cpp
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt
LIBDPLIBS+= m  ${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+= pthread ${NETBSDSRCDIR}/lib/libpthread
CPPFLAGS+=-DCAN_SANITIZE_UB=0

.if ${MACHINE_ARCH} == "vax"
COPTS.asan_allocator.cpp += -O1
COPTS.asan_report.cpp += -O1
COPTS.sanitizer_file.cpp += -O1
COPTS.ubsan_diag.cpp += -O1
COPTS.ubsan_init.cpp += -O1
.endif

.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
COPTS.asan_interceptors.cpp += -O1
.endif

.include <bsd.lib.mk>
