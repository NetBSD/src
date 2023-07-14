#	$NetBSD: Makefile,v 1.7 2023/07/14 19:46:40 mrg Exp $

UNSUPPORTED_COMPILER.clang=	# defined
LIBISCXX = yes

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/ubsan

# XXXmknative
UBSAN_SRCS= \
	ubsan_diag.cc \
	ubsan_flags.cc \
	ubsan_handlers.cc \
	ubsan_init.cc \
	ubsan_monitor.cc \
	ubsan_type_hash.cc \
	ubsan_type_hash_itanium.cc \
	ubsan_type_hash_win.cc \
	ubsan_value.cc

.for _s in ubsan_handlers_cxx ubsan_type_hash ubsan_type_hash_itanium
COPTS.${_s}.cc+=-frtti
.endfor

.if ${MACHINE_ARCH} == "vax"
COPTS.sanitizer_file.cc += -O1
.endif

LIB=	ubsan
SRCS+=	${UBSAN_SRCS}
LIBDPLIBS+= m ${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt
LIBDPLIBS+= pthread ${NETBSDSRCDIR}/lib/libpthread
CPPFLAGS+=-DCAN_SANITIZE_UB=1

# XXX
COPTS.ubsan_handlers.cc+=${CC_WNO_MAYBE_UNINITIALIZED}

.include <bsd.lib.mk>
