#	$NetBSD: Makefile,v 1.1.1.2 2018/11/11 21:00:04 mrg Exp $

.include "../../../Makefile.gcc_path"

NOOBJ=	#defined

SANITIZER=	${GCCDIST}/libsanitizer

.PATH:	${SANITIZER}/include/sanitizer

INCS=	#
INCS+=	allocator_interface.h
INCS+=	asan_interface.h
INCS+=	common_interface_defs.h
#INCS+=	dfsan_interface.h # not supported
#INCS+=	lsan_interface.h # not supported
#INCS+=	msan_interface.h # not ready in GCC
INCS+=	tsan_interface_atomic.h

.include <bsd.own.mk>

INCSDIR=	${GCC_INCSDIR}/sanitizer

.include <bsd.prog.mk>
