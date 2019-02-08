#	$NetBSD: Makefile,v 1.11 2019/02/08 22:06:12 mrg Exp $

UNSUPPORTED_COMPILER.clang=	# defined
LIBISCXX = yes

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/ubsan

UBSAN_SRCS= \
	ubsan_diag.cc \
	ubsan_flags.cc \
	ubsan_handlers.cc \
	ubsan_handlers_cxx.cc \
	ubsan_init.cc \
	ubsan_type_hash.cc \
	ubsan_type_hash_itanium.cc \
	ubsan_type_hash_win.cc \
	ubsan_value.cc \
	ubsan_init_standalone.cc

.for _s in ubsan_handlers_cxx ubsan_type_hash ubsan_type_hash_itanium
COPTS.${_s}.cc+=-frtti
.endfor

COPTS.ubsan_handlers_cxx.cc+= -Wno-error=maybe-uninitialized

LIB=	ubsan
SRCS+=	${UBSAN_SRCS}
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt
CPPFLAGS+=-DCAN_SANITIZE_UB=1

.include <bsd.lib.mk>
