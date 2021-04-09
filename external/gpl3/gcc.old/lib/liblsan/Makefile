# $NetBSD: Makefile,v 1.5 2021/04/09 23:15:37 mrg Exp $

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

.if ${MACHINE_ARCH} == "vax"
COPTS.lsan_allocator.cc += -O1
COPTS.sanitizer_file.cc += -O1
.endif

LIB=	lsan
SRCS+=	${LSAN_SRCS}
LIBDPLIBS+= m ${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt
LIBDPLIBS+= pthread ${NETBSDSRCDIR}/lib/libpthread

CWARNFLAGS.clang+=	-Wno-error=unused-private-field

.include <bsd.lib.mk>
