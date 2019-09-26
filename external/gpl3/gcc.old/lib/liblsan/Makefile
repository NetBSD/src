# $NetBSD: Makefile,v 1.1.1.2 2019/09/26 05:45:54 mrg Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/lsan ${SANITIZER}/asan

LSAN_SRCS+= \
	lsan_common.cc \
	lsan_common_linux.cc \
	lsan.cc \
	lsan_allocator.cc \
	lsan_interceptors.cc \
	lsan_thread.cc

.if ${MACHINE_ARCH} == "vax"
COPTS.lsan_allocator.cc += -O1
.endif

LIB=	lsan
SRCS+=	${LSAN_SRCS}
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt

.include <bsd.lib.mk>
