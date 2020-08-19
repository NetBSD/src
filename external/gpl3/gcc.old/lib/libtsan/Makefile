# $NetBSD: Makefile,v 1.9 2020/08/19 07:48:02 mrg Exp $

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

# XXX ${MACHINE} won't match mips64 or ppc64
.if exists(${SANITIZER}/tsan/tsan_rtl_${MACHINE}.S)
TSAN_SRCS+= tsan_rtl_${MACHINE}.S
.else
TSAN_SRCS+= tsan_rtl_unimpl.cc
.endif

LIB=	tsan
SRCS+=	${TSAN_SRCS}
CPPFLAGS+=-DCAN_SANITIZE_UB=0

.include <bsd.lib.mk>
