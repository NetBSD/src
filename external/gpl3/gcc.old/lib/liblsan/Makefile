# $NetBSD: Makefile,v 1.8 2025/03/25 18:03:32 christos Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/lsan ${SANITIZER}/asan

LSAN_SRCS+= \
	lsan.cc \
	lsan_allocator.cc \
	lsan_common.cc \
	lsan_common_linux.cc \
	lsan_interceptors.cc \
	lsan_linux.cc \
	lsan_thread.cc

LIB=	lsan
SRCS+=	${LSAN_SRCS}
LIBDPLIBS+= m ${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt
LIBDPLIBS+= pthread ${NETBSDSRCDIR}/lib/libpthread

CWARNFLAGS.clang+=	-Wno-error=unused-private-field

.include "../preinit.mk"

.include <bsd.lib.mk>
