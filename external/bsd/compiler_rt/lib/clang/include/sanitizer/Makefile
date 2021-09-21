#	$NetBSD: Makefile,v 1.4 2021/09/21 20:34:37 christos Exp $

NOOBJ=	# defined

.include <bsd.init.mk>

.PATH:	${TOPDIR}/include/sanitizer

INCS+=	allocator_interface.h
INCS+=	asan_interface.h
INCS+=	common_interface_defs.h
INCS+=	coverage_interface.h
#INCS+=	dfsan_interface.h # not supported
#INCS+=	esan_interface.h # not supported
#INCS+=	hwasan_interface.h # not supported
INCS+=	lsan_interface.h
INCS+=	msan_interface.h
INCS+=	netbsd_syscall_hooks.h
#INCS+=	scudo_interface.h # not ready
INCS+=	tsan_interface.h
INCS+=	tsan_interface_atomic.h

INCSDIR=	/usr/lib/clang/${CLANG_VER}/include/sanitizer

.include <bsd.prog.mk>
