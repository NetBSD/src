#	$NetBSD: Makefile,v 1.3 2017/06/29 18:52:23 joerg Exp $

UNSUPPORTED_COMPILER.clang=	# defined

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
	ubsan_type_hash.o \
	ubsan_value.o

COPTS.ubsan_type_hash.cc+=-frtti

LIB=	ubsan
SRCS+=	${UBSAN_SRCS}

.if ${MACHINE_ARCH} == "vax"
COPTS += -O1
.endif

.include <bsd.lib.mk>
