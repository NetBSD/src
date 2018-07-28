#	$NetBSD: Makefile,v 1.5.2.2 2018/07/28 04:37:18 pgoyette Exp $

UNSUPPORTED_COMPILER.clang=	# defined
LIBISCXX = yes

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/ubsan

UBSAN_SRCS= \
	ubsan_diag.o \
	ubsan_flags.o \
	ubsan_handlers.o \
	ubsan_handlers_cxx.o \
	ubsan_init.o \
	ubsan_init_standalone.o \
	ubsan_type_hash.o \
	ubsan_type_hash_itanium.o \
	ubsan_type_hash_win.o \
	ubsan_value.o


.for _s in ubsan_handlers_cxx ubsan_type_hash ubsan_type_hash_itanium
COPTS.${_s}.cc+=-frtti
.endfor

LIB=	ubsan
SRCS+=	${UBSAN_SRCS}
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt

.include <bsd.lib.mk>
