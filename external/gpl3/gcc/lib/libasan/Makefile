
.include <bsd.own.mk>

.include "../Makefile.inc"

ASAN=${GCCDIST}/libsanitizer
.PATH: ${ASAN}/asan ${ASAN}/interception ${ASAN}/sanitizer_common

ASAN_SRCS= \
asan_activation.cc \
asan_allocator2.cc \
asan_debugging.cc \
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
asan_thread.cc \
asan_win.cc \
asan_win_dll_thunk.cc \
asan_win_dynamic_runtime_thunk.cc

INTERCEPTION_SRCS= \
interception_linux.cc \
interception_mac.cc \
interception_type_test.cc \
interception_win.cc

SANITIZER_SRCS= \
sanitizer_allocator.cc \
sanitizer_common.cc \
sanitizer_common_libcdep.cc \
sanitizer_coverage_libcdep.cc \
sanitizer_coverage_mapping_libcdep.cc \
sanitizer_deadlock_detector1.cc \
sanitizer_deadlock_detector2.cc \
sanitizer_flags.cc \
sanitizer_libc.cc \
sanitizer_libignore.cc \
sanitizer_linux.cc \
sanitizer_linux_libcdep.cc \
sanitizer_mac.cc \
sanitizer_persistent_allocator.cc \
sanitizer_platform_limits_linux.cc \
sanitizer_platform_limits_posix.cc \
sanitizer_posix.cc \
sanitizer_posix_libcdep.cc \
sanitizer_printf.cc \
sanitizer_procmaps_common.cc \
sanitizer_procmaps_netbsd.cc \
sanitizer_procmaps_freebsd.cc \
sanitizer_procmaps_linux.cc \
sanitizer_procmaps_mac.cc \
sanitizer_stackdepot.cc \
sanitizer_stacktrace.cc \
sanitizer_stacktrace_libcdep.cc \
sanitizer_stacktrace_printer.cc \
sanitizer_stoptheworld_linux_libcdep.cc \
sanitizer_suppressions.cc \
sanitizer_symbolizer.cc \
sanitizer_symbolizer_libbacktrace.cc \
sanitizer_symbolizer_libcdep.cc \
sanitizer_symbolizer_posix_libcdep.cc \
sanitizer_symbolizer_win.cc \
sanitizer_thread_registry.cc \
sanitizer_tls_get_addr.cc \
sanitizer_unwind_posix_libcdep.cc \
sanitizer_win.cc

# The linux build does this to avoid preinit sections on shared libraries
CSHLIBFLAGS+= -DPIC

LIB=	asan
SRCS=	${ASAN_SRCS} ${INTERCEPTION_SRCS} ${SANITIZER_SRCS}
CPPFLAGS+=-I${ASAN}/include -I${ASAN}
CPPFLAGS.sanitizer_netbsd.cc+=-I${GCCDIST}/gcc/ginclude
CPPFLAGS+=-D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -DASAN_HAS_EXCEPTIONS=1 -DASAN_FLEXIBLE_MAPPING_AND_OFFSET=0 -DASAN_NEEDS_SEGV=1

LIBDPLIBS+=    stdc++ ${.CURDIR}/../libstdc++-v3
LIBDPLIBS+=    pthread ${.CURDIR}/../../../../../lib/libpthread
COPTS+=-std=gnu++11 -fsized-deallocation -fvisibility=hidden

.if ${MACHINE_ARCH} == "vax"
COPTS.asan_allocator2.cc += -O1
COPTS.sanitizer_mac.cc += -O1
COPTS.sanitizer_netbsd.cc += -O1
COPTS.sanitizer_printf.cc += -O1
COPTS.sanitizer_stackdepot.cc += -O1
.endif

.include <bsd.lib.mk>
