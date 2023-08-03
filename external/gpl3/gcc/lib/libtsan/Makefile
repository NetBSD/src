# $NetBSD: Makefile,v 1.17 2023/08/03 01:36:55 mrg Exp $

UNSUPPORTED_COMPILER.clang=     # defined
NOSANITIZER=    # defined
NOCOMPAT=	# defined

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/tsan

# XXXmknative
TSAN_SRCS= \
	tsan_clock.cpp \
	tsan_debugging.cpp \
	tsan_external.cpp \
	tsan_fd.cpp \
	tsan_flags.cpp \
	tsan_ignoreset.cpp \
	tsan_interceptors_posix.cpp \
	tsan_interceptors_mac.cpp \
	tsan_interface_ann.cpp \
	tsan_interface_atomic.cpp \
	tsan_interface.cpp \
	tsan_interface_java.cpp \
	tsan_malloc_mac.cpp \
	tsan_md5.cpp \
	tsan_mman.cpp \
	tsan_mutexset.cpp \
	tsan_new_delete.cpp \
	tsan_platform_linux.cpp \
	tsan_platform_mac.cpp \
	tsan_platform_posix.cpp \
	tsan_platform_windows.cpp \
	tsan_report.cpp \
	tsan_rtl.cpp \
	tsan_rtl_access.cpp \
	tsan_rtl_mutex.cpp \
	tsan_rtl_proc.cpp \
	tsan_rtl_report.cpp \
	tsan_rtl_thread.cpp \
	tsan_stack_trace.cpp \
	tsan_suppressions.cpp \
	tsan_symbolize.cpp \
	tsan_sync.cpp \
	tsan_vector_clock.cpp

.if ${MACHINE_ARCH} == "x86_64"
TSAN_SRCS+= tsan_rtl_amd64.S
.elif ${MACHINE_CPU} == "aarch64"
TSAN_SRCS+= tsan_rtl_aarch64.S
.elif ${MACHINE_MIPS64}
TSAN_SRCS+= tsan_rtl_mips64.S
.elif ${MACHINE_ARCH} == "powerpc64"
TSAN_SRCS+= tsan_rtl_ppc64.S
.else
.error Unsupported
.endif

LIB=	tsan
SRCS+=	${TSAN_SRCS}
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt
LIBDPLIBS+= m  ${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+= pthread ${NETBSDSRCDIR}/lib/libpthread
CPPFLAGS+=-DCAN_SANITIZE_UB=0
COPTS.tsan_interceptors.cpp += -Wno-unused-function

.include <bsd.lib.mk>
