# $NetBSD: Makefile,v 1.4 2016/06/01 22:48:55 christos Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/tsan

TSAN_SRCS= \
tsan_clock.o \
tsan_fd.o \
tsan_flags.o \
tsan_ignoreset.o \
tsan_interceptors.o \
tsan_interface.o \
tsan_interface_ann.o \
tsan_interface_atomic.o \
tsan_interface_java.o \
tsan_md5.o \
tsan_mman.o \
tsan_mutex.o \
tsan_mutexset.o \
tsan_platform_linux.o \
tsan_platform_mac.o \
tsan_platform_windows.o \
tsan_report.o \
tsan_rtl.o \
tsan_rtl_mutex.o \
tsan_rtl_report.o \
tsan_rtl_thread.o \
tsan_stack_trace.o \
tsan_stat.o \
tsan_suppressions.o \
tsan_symbolize.o \
tsan_sync.o

LIB=	tsan
SRCS+=	${TSAN_SRCS}

.include <bsd.lib.mk>
