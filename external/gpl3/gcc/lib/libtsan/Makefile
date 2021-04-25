# $NetBSD: Makefile,v 1.16 2021/04/25 23:12:54 christos Exp $

UNSUPPORTED_COMPILER.clang=     # defined
NOSANITIZER=    # defined
NOCOMPAT=	# defined

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/tsan

# XXXmknative
TSAN_SRCS= \
	tsan_clock.cc \
	tsan_debugging.cc \
	tsan_external.cc \
	tsan_fd.cc \
	tsan_flags.cc \
	tsan_ignoreset.cc \
	tsan_interceptors.cc \
	tsan_interceptors_mac.cc \
	tsan_interface_ann.cc \
	tsan_interface_atomic.cc \
	tsan_interface.cc \
	tsan_interface_java.cc \
	tsan_libdispatch_mac.cc \
	tsan_malloc_mac.cc \
	tsan_md5.cc \
	tsan_mman.cc \
	tsan_mutex.cc \
	tsan_mutexset.cc \
	tsan_new_delete.cc \
	tsan_platform_linux.cc \
	tsan_platform_mac.cc \
	tsan_platform_posix.cc \
	tsan_platform_windows.cc \
	tsan_report.cc \
	tsan_rtl.cc \
	tsan_rtl_mutex.cc \
	tsan_rtl_proc.cc \
	tsan_rtl_report.cc \
	tsan_rtl_thread.cc \
	tsan_stack_trace.cc \
	tsan_stat.cc \
	tsan_suppressions.cc \
	tsan_symbolize.cc \
	tsan_sync.cc 

.if ${MACHINE_ARCH} == "x86_64"
TSAN_SRCS+= tsan_rtl_amd64.S
.elif ${MACHINE_ARCH} == "aarch64"
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
COPTS.tsan_interceptors.cc += -Wno-unused-function

.include <bsd.lib.mk>
