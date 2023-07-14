#	$NetBSD: Makefile,v 1.1.1.1 2023/07/14 06:34:27 mrg Exp $

.include "../../../Makefile.gcc_path"

NOOBJ=	#defined

SANITIZER=	${GCCDIST}/libsanitizer

.PATH:	${SANITIZER}/include/sanitizer

INCS=	#
INCS+=	allocator_interface.h
INCS+=	asan_interface.h
INCS+=	common_interface_defs.h
#INCS+=	dfsan_interface.h # not supported
INCS+=	lsan_interface.h
#INCS+=	msan_interface.h # not ready in GCC
INCS+=	netbsd_syscall_hooks.h
INCS+=	tsan_interface.h
INCS+=	tsan_interface_atomic.h

.include <bsd.own.mk>

INCSDIR=	${GCC_INCSDIR}/sanitizer

.include <bsd.prog.mk>
