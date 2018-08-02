#	$NetBSD: Makefile,v 1.2 2018/08/02 07:47:30 mrg Exp $

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

INCSDIR=	/usr/include/gcc-6/sanitizer

.include <bsd.prog.mk>
