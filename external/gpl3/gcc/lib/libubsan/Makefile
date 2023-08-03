#	$NetBSD: Makefile,v 1.21 2023/08/03 01:36:55 mrg Exp $

UNSUPPORTED_COMPILER.clang=	# defined
LIBISCXX = yes

.include <bsd.own.mk>

.include "../Makefile.inc"
.include "../Makefile.sanitizer"

.PATH: ${SANITIZER}/ubsan

# XXXmknative
UBSAN_SRCS= \
	ubsan_diag.cpp \
	ubsan_flags.cpp \
	ubsan_handlers.cpp \
	ubsan_handlers_cxx.cpp \
	ubsan_init.cpp \
	ubsan_monitor.cpp \
	ubsan_type_hash.cpp \
	ubsan_type_hash_itanium.cpp \
	ubsan_type_hash_win.cpp \
	ubsan_value.cpp

.for _s in ubsan_handlers_cxx ubsan_type_hash ubsan_type_hash_itanium
COPTS.${_s}.cpp+=-frtti
.endfor

.if ${MACHINE_ARCH} == "vax"
COPTS.sanitizer_file.cpp += -O1
.endif

LIB=	ubsan
SRCS+=	${UBSAN_SRCS}
LIBDPLIBS+= m ${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+= rt ${NETBSDSRCDIR}/lib/librt
LIBDPLIBS+= pthread ${NETBSDSRCDIR}/lib/libpthread
CPPFLAGS+=-DCAN_SANITIZE_UB=1 -DUBSAN_CAN_USE_CXXABI=1

# XXX
COPTS.ubsan_handlers.cpp+=${CC_WNO_MAYBE_UNINITIALIZED}

.include <bsd.lib.mk>
