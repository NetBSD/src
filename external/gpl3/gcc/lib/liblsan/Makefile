# $NetBSD: Makefile,v 1.1 2018/06/26 20:54:32 christos Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/lsan ${SANITIZER}/asan

LSAN_SRCS+= \
sanitizer_stoptheworld_linux_libcdep.cc \
lsan.cc \
lsan_allocator.cc \
lsan_common.cc \
lsan_common_linux.cc \
lsan_interceptors.cc \
lsan_preinit.cc \
lsan_thread.cc


LIB=	lsan
SRCS+=	${LSAN_SRCS}
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt

.include <bsd.lib.mk>
