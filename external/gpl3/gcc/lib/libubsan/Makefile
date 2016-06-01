#	$NetBSD: Makefile,v 1.1 2016/06/01 22:48:55 christos Exp $
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

.include <bsd.lib.mk>
