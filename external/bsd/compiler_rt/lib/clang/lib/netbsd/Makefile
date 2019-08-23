#	$NetBSD: Makefile,v 1.3 2019/08/23 06:38:27 kamil Exp $

.include <bsd.own.mk>

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	asan-m64
SUBDIR+=	asan-preinit-m64
SUBDIR+=	asan_cxx-m64
SUBDIR+=	asan-dynamic-m64
SUBDIR+=	dd-m64
SUBDIR+=	dyndd-m64
SUBDIR+=	fuzzer-m64
SUBDIR+=	fuzzer_no_main-m64
SUBDIR+=	msan-m64
SUBDIR+=	msan_cxx-m64
SUBDIR+=	safestack-m64
SUBDIR+=	tsan-m64
SUBDIR+=	tsan_cxx-m64
SUBDIR+=	ubsan_minimal-m64
SUBDIR+=	ubsan_minimal-dynamic-m64
SUBDIR+=	ubsan_standalone-m64
SUBDIR+=	ubsan_standalone-dynamic-m64
SUBDIR+=	ubsan_standalone_cxx-m64
SUBDIR+=	xray-m64
SUBDIR+=	xray-basic-m64
SUBDIR+=	xray-fdr-m64
SUBDIR+=	xray-profiling-m64
.endif

.if ${MACHINE_ARCH} == "i386" || \
        (${MACHINE_ARCH} == "x86_64" && ${MKCOMPAT} != "no")
SUBDIR+=	asan-m32
SUBDIR+=	asan-preinit-m32
SUBDIR+=	asan_cxx-m32
SUBDIR+=	asan-dynamic-m32
SUBDIR+=	safestack-m32
SUBDIR+=	ubsan_minimal-m32
SUBDIR+=	ubsan_minimal-dynamic-m32
SUBDIR+=	ubsan_standalone-m32
SUBDIR+=	ubsan_standalone-dynamic-m32
SUBDIR+=	ubsan_standalone_cxx-m32
.endif

.include <bsd.subdir.mk>
