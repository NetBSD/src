#	$NetBSD: Makefile,v 1.2 2016/06/09 07:11:08 mrg Exp $
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
