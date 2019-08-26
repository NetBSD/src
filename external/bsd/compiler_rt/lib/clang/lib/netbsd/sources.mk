#	$NetBSD: sources.mk,v 1.2 2019/08/26 04:49:45 kamil Exp $

# RTInterception
INTERCEPTION_SOURCES+=	interception_linux.cc
INTERCEPTION_SOURCES+=	interception_mac.cc
INTERCEPTION_SOURCES+=	interception_win.cc
INTERCEPTION_SOURCES+=	interception_type_test.cc

# RTSanitizerCommonNoTermination
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_allocator.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_common.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_deadlock_detector1.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_deadlock_detector2.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_errno.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_file.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_flags.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_flag_parser.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_fuchsia.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_libc.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_libignore.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_linux.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_linux_s390.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_mac.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_netbsd.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_openbsd.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_persistent_allocator.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_platform_limits_freebsd.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_platform_limits_linux.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_platform_limits_netbsd.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_platform_limits_openbsd.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_platform_limits_posix.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_platform_limits_solaris.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_posix.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_printf.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_procmaps_common.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_procmaps_bsd.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_procmaps_linux.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_procmaps_mac.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_procmaps_solaris.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_rtems.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_solaris.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_stoptheworld_mac.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_suppressions.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_tls_get_addr.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_thread_registry.cc
SANITIZER_SOURCES_NOTERMINATION+=	sanitizer_win.cc

# RTSanitizerCommon
SANITIZER_SOURCES+=	${SANITIZER_SOURCES_NOTERMINATION}
SANITIZER_SOURCES+=	sanitizer_termination.cc

# RTSanitizerCommonNoLibc
SANITIZER_NOLIBC_SOURCES+=	sanitizer_common_nolibc.cc

.for w in ${SANITIZER_NOLIBC_SOURCES}
COPTS.${w}+=	-fno-rtti
.endfor

# RTSanitizerCommonLibc
SANITIZER_LIBCDEP_SOURCES+=	sanitizer_common_libcdep.cc
SANITIZER_LIBCDEP_SOURCES+=	sanitizer_allocator_checks.cc
SANITIZER_LIBCDEP_SOURCES+=	sanitizer_linux_libcdep.cc
SANITIZER_LIBCDEP_SOURCES+=	sanitizer_mac_libcdep.cc
SANITIZER_LIBCDEP_SOURCES+=	sanitizer_posix_libcdep.cc
SANITIZER_LIBCDEP_SOURCES+=	sanitizer_stoptheworld_linux_libcdep.cc

# RTSanitizerCommonCoverage
SANITIZER_COVERAGE_SOURCES+=	sancov_flags.cc
SANITIZER_COVERAGE_SOURCES+=	sanitizer_coverage_fuchsia.cc
SANITIZER_COVERAGE_SOURCES+=	sanitizer_coverage_libcdep_new.cc
SANITIZER_COVERAGE_SOURCES+=	sanitizer_coverage_win_sections.cc

# RTSanitizerCommonSymbolizer
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_allocator_report.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_stackdepot.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_stacktrace.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_stacktrace_libcdep.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_stacktrace_printer.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_stacktrace_sparc.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer_libbacktrace.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer_libcdep.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer_mac.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer_markup.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer_posix_libcdep.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer_report.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer_win.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_unwind_linux_libcdep.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer_report.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_symbolizer_win.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_unwind_linux_libcdep.cc
SANITIZER_SYMBOLIZER_SOURCES+=	sanitizer_unwind_win.cc

DD_SOURCES+=	dd_rtl.cc
DD_SOURCES+=	dd_interceptors.cc

ASAN_SOURCES+=	asan_allocator.cc
ASAN_SOURCES+=	asan_activation.cc
ASAN_SOURCES+=	asan_debugging.cc
ASAN_SOURCES+=	asan_descriptions.cc
ASAN_SOURCES+=	asan_errors.cc
ASAN_SOURCES+=	asan_fake_stack.cc
ASAN_SOURCES+=	asan_flags.cc
ASAN_SOURCES+=	asan_fuchsia.cc
ASAN_SOURCES+=	asan_globals.cc
ASAN_SOURCES+=	asan_globals_win.cc
ASAN_SOURCES+=	asan_interceptors.cc
ASAN_SOURCES+=	asan_interceptors_memintrinsics.cc
ASAN_SOURCES+=	asan_linux.cc
ASAN_SOURCES+=	asan_mac.cc
ASAN_SOURCES+=	asan_malloc_linux.cc
ASAN_SOURCES+=	asan_malloc_mac.cc
ASAN_SOURCES+=	asan_malloc_win.cc
ASAN_SOURCES+=	asan_memory_profile.cc
ASAN_SOURCES+=	asan_poisoning.cc
ASAN_SOURCES+=	asan_posix.cc
ASAN_SOURCES+=	asan_premap_shadow.cc
ASAN_SOURCES+=	asan_report.cc
ASAN_SOURCES+=	asan_rtems.cc
ASAN_SOURCES+=	asan_rtl.cc
ASAN_SOURCES+=	asan_shadow_setup.cc
ASAN_SOURCES+=	asan_stack.cc
ASAN_SOURCES+=	asan_stats.cc
ASAN_SOURCES+=	asan_suppressions.cc
ASAN_SOURCES+=	asan_thread.cc
ASAN_SOURCES+=	asan_win.cc

ASAN_CXX_SOURCES+=	asan_new_delete.cc

ASAN_PREINIT_SOURCES+=	asan_preinit.cc

LIBFUZZER_SOURCES+=	FuzzerCrossOver.cpp
LIBFUZZER_SOURCES+=	FuzzerDataFlowTrace.cpp
LIBFUZZER_SOURCES+=	FuzzerDriver.cpp
LIBFUZZER_SOURCES+=	FuzzerExtFunctionsDlsym.cpp
LIBFUZZER_SOURCES+=	FuzzerExtFunctionsWeakAlias.cpp
LIBFUZZER_SOURCES+=	FuzzerExtFunctionsWeak.cpp
LIBFUZZER_SOURCES+=	FuzzerExtraCounters.cpp
LIBFUZZER_SOURCES+=	FuzzerIO.cpp
LIBFUZZER_SOURCES+=	FuzzerIOPosix.cpp
LIBFUZZER_SOURCES+=	FuzzerIOWindows.cpp
LIBFUZZER_SOURCES+=	FuzzerLoop.cpp
LIBFUZZER_SOURCES+=	FuzzerMerge.cpp
LIBFUZZER_SOURCES+=	FuzzerMutate.cpp
LIBFUZZER_SOURCES+=	FuzzerSHA1.cpp
LIBFUZZER_SOURCES+=	FuzzerShmemFuchsia.cpp
LIBFUZZER_SOURCES+=	FuzzerShmemPosix.cpp
LIBFUZZER_SOURCES+=	FuzzerShmemWindows.cpp
LIBFUZZER_SOURCES+=	FuzzerTracePC.cpp
LIBFUZZER_SOURCES+=	FuzzerUtil.cpp
LIBFUZZER_SOURCES+=	FuzzerUtilDarwin.cpp
LIBFUZZER_SOURCES+=	FuzzerUtilFuchsia.cpp
LIBFUZZER_SOURCES+=	FuzzerUtilLinux.cpp
LIBFUZZER_SOURCES+=	FuzzerUtilPosix.cpp
LIBFUZZER_SOURCES+=	FuzzerUtilWindows.cpp

LIBFUZZER_MAIN_SOURCES+=	FuzzerMain.cpp

MSAN_RTL_SOURCES+=	msan.cc
MSAN_RTL_SOURCES+=	msan_allocator.cc
MSAN_RTL_SOURCES+=	msan_chained_origin_depot.cc
MSAN_RTL_SOURCES+=	msan_interceptors.cc
MSAN_RTL_SOURCES+=	msan_linux.cc
MSAN_RTL_SOURCES+=	msan_report.cc
MSAN_RTL_SOURCES+=	msan_thread.cc
MSAN_RTL_SOURCES+=	msan_poisoning.cc

MSAN_RTL_CXX_SOURCES+=	msan_new_delete.cc

SAFESTACK_SOURCES+=	safestack.cc

STATS_SOURCES+=	stats.cc

STATS_CLIENT_SOURCES+=	stats_client.cc

TSAN_SOURCES+=	tsan_clock.cc
TSAN_SOURCES+=	tsan_debugging.cc
TSAN_SOURCES+=	tsan_external.cc
TSAN_SOURCES+=	tsan_fd.cc
TSAN_SOURCES+=	tsan_flags.cc
TSAN_SOURCES+=	tsan_ignoreset.cc
TSAN_SOURCES+=	tsan_interceptors.cc
TSAN_SOURCES+=	tsan_interface.cc
TSAN_SOURCES+=	tsan_interface_ann.cc
TSAN_SOURCES+=	tsan_interface_atomic.cc
TSAN_SOURCES+=	tsan_interface_java.cc
TSAN_SOURCES+=	tsan_malloc_mac.cc
TSAN_SOURCES+=	tsan_md5.cc
TSAN_SOURCES+=	tsan_mman.cc
TSAN_SOURCES+=	tsan_mutex.cc
TSAN_SOURCES+=	tsan_mutexset.cc
TSAN_SOURCES+=	tsan_preinit.cc
TSAN_SOURCES+=	tsan_report.cc
TSAN_SOURCES+=	tsan_rtl.cc
TSAN_SOURCES+=	tsan_rtl_mutex.cc
TSAN_SOURCES+=	tsan_rtl_proc.cc
TSAN_SOURCES+=	tsan_rtl_report.cc
TSAN_SOURCES+=	tsan_rtl_thread.cc
TSAN_SOURCES+=	tsan_stack_trace.cc
TSAN_SOURCES+=	tsan_stat.cc
TSAN_SOURCES+=	tsan_suppressions.cc
TSAN_SOURCES+=	tsan_symbolize.cc
TSAN_SOURCES+=	tsan_sync.cc
TSAN_SOURCES+=	tsan_platform_linux.cc
TSAN_SOURCES+=	tsan_platform_posix.cc

TSAN_CXX_SOURCES+=	tsan_new_delete.cc

.include <bsd.own.mk>

.if ${MACHINE_ARCH} == "x86_64"
TSAN_ASM_SOURCES+=	tsan_rtl_amd64.S
.endif

UBSAN_MINIMAL_SOURCES+=	ubsan_minimal_handlers.cc

UBSAN_SOURCES+=	ubsan_diag.cc
UBSAN_SOURCES+=	ubsan_init.cc
UBSAN_SOURCES+=	ubsan_flags.cc
UBSAN_SOURCES+=	ubsan_handlers.cc
UBSAN_SOURCES+=	ubsan_monitor.cc
UBSAN_SOURCES+=	ubsan_value.cc

UBSAN_STANDALONE_SOURCES+=	ubsan_diag_standalone.cc
UBSAN_STANDALONE_SOURCES+=	ubsan_init_standalone.cc
UBSAN_STANDALONE_SOURCES+=	ubsan_signals_standalone.cc

UBSAN_STANDALONE_SOURCES_STATIC+=	ubsan_init_standalone_preinit.cc

UBSAN_CXXABI_SOURCES+=	ubsan_handlers_cxx.cc
UBSAN_CXXABI_SOURCES+=	ubsan_type_hash.cc
UBSAN_CXXABI_SOURCES+=	ubsan_type_hash_itanium.cc
UBSAN_CXXABI_SOURCES+=	ubsan_type_hash_win.cc

.if 0 # ${MKCXXABI:U} == "yes"
UBSAN_CXX_SOURCES+=	${UBSAN_CXXABI_SOURCES}
.else
UBSAN_CXX_SOURCES+=	cxx_dummy.cc
CLEANFILES+=		cxx_dummy.cc

cxx_dummy.cc:
	touch ${.TARGET}
.endif

XRAY_SOURCES+=	xray_buffer_queue.cc
XRAY_SOURCES+=	xray_init.cc
XRAY_SOURCES+=	xray_flags.cc
XRAY_SOURCES+=	xray_interface.cc
XRAY_SOURCES+=	xray_log_interface.cc
XRAY_SOURCES+=	xray_utils.cc

XRAY_FDR_MODE_SOURCES+=	xray_fdr_flags.cc
XRAY_FDR_MODE_SOURCES+=	xray_fdr_logging.cc

XRAY_BASIC_MODE_SOURCES+=	xray_basic_flags.cc  
XRAY_BASIC_MODE_SOURCES+=	xray_basic_logging.cc

XRAY_PROFILING_MODE_SOURCES+=	xray_profile_collector.cc
XRAY_PROFILING_MODE_SOURCES+=	xray_profiling.cc
XRAY_PROFILING_MODE_SOURCES+=	xray_profiling_flags.cc

XRAY_X86_64_SOURCES+=	xray_x86_64.cc
XRAY_X86_64_SOURCES+=	xray_trampoline_x86_64.S

XRAY_ARM_SOURCES+=	xray_arm.cc
XRAY_ARM_SOURCES+=	xray_trampoline_arm.S

XRAY_ARMHF_SOURCES+=	${XRAY_ARM_SOURCES}

XRAY_AARCH64_SOURCES+=	xray_AArch64.cc
XRAY_AARCH64_SOURCES+=	xray_trampoline_AArch64.S

XRAY_MIPS_SOURCES+=	xray_mips.cc
XRAY_MIPS_SOURCES+=	xray_trampoline_mips.S

XRAY_MIPSEL_SOURCES+=	xray_mips.cc
XRAY_MIPSEL_SOURCES+=	xray_trampoline_mips.S

XRAY_MIPS64_SOURCES+=	xray_mips64.cc
XRAY_MIPS64_SOURCES+=	xray_trampoline_mips64.S

XRAY_MIPS64EL_SOURCES+=	xray_mips64.cc
XRAY_MIPS64EL_SOURCES+=	xray_trampoline_mips64.S

XRAY_POWERPC64LE_SOURCES+=	xray_powerpc64.cc
XRAY_POWERPC64LE_SOURCES+=	xray_trampoline_powerpc64.cc
XRAY_POWERPC64LE_SOURCES+=	xray_trampoline_powerpc64_asm.S

.if ${MACHINE_ARCH} == "x86_64"
XRAY_ARCH_SOURCES+=	${XRAY_X86_64_SOURCES}
.endif

LSAN_COMMON_SOURCES+=	lsan_common.cc
LSAN_COMMON_SOURCES+=	lsan_common_linux.cc
LSAN_COMMON_SOURCES+=	lsan_common_mac.cc

LSAN_SOURCES+=		lsan.cc
LSAN_SOURCES+=		lsan_allocator.cc
LSAN_SOURCES+=		lsan_linux.cc
LSAN_SOURCES+=		lsan_interceptors.cc
LSAN_SOURCES+=		lsan_mac.cc
LSAN_SOURCES+=		lsan_malloc_mac.cc
LSAN_SOURCES+=		lsan_preinit.cc
LSAN_SOURCES+=		lsan_thread.cc
