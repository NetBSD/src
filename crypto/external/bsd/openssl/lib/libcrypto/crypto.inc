#	$NetBSD: crypto.inc,v 1.7 2018/03/03 04:32:46 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/Makefile
#

.PATH:	${OPENSSLSRC}/crypto


CRYPTO_SRCS += \
cpt_err.c \
cryptlib.c \
cversion.c \
ebcdic.c \
ex_data.c \
init.c \
mem.c \
mem_dbg.c \
mem_sec.c \
o_dir.c \
o_fips.c \
o_fopen.c \
o_init.c \
o_str.c \
o_time.c \
threads_none.c \
threads_pthread.c \
threads_win.c \
uid.c

SRCS += ${CRYPTO_SRCS}

.if !defined(CPUID)
CRYPTO_SRCS+=  mem_clr.c
.else
CRYPTO_SRCS+=  ${CPUID_SRCS}
#CRYPTOCPPFLAGS=-DOPENSSL_CPUID_OBJ
.endif

CRYPTOCPPFLAGS+=-DOPENSSL_NO_STATIC_ENGINE

.for cryptosrc in ${CRYPTO_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/crypto ${CRYPTOCPPFLAGS}
.endfor
