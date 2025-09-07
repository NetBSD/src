# $NetBSD: Makefile,v 1.9 2025/09/07 04:12:11 mrg Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/lsan ${SANITIZER}/asan

LSAN_SRCS+= \
	lsan_common.cpp \
	lsan_common_linux.cpp \
	lsan_common_mac.cpp \
	lsan.cpp \
	lsan_linux.cpp \
	lsan_mac.cpp \
	lsan_malloc_mac.cpp \
	lsan_allocator.cpp \
	lsan_interceptors.cpp \
	lsan_thread.cpp \
	lsan_posix.cpp

LIB=	lsan
SRCS+=	${LSAN_SRCS}
LIBDPLIBS+= m ${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt
LIBDPLIBS+= pthread ${NETBSDSRCDIR}/lib/libpthread

CWARNFLAGS.clang+=	-Wno-error=unused-private-field

.include "../preinit.mk"

.include <bsd.lib.mk>
