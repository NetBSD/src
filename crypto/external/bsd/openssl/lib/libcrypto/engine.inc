#	$NetBSD: engine.inc,v 1.11 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/engine/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/engine ${OPENSSLSRC}/engines \
    ${OPENSSLSRC}/engines/ccgost


ENGINE_SRCS += \
eng_all.c \
eng_cnf.c \
eng_cryptodev.c \
eng_ctrl.c \
eng_dyn.c \
eng_err.c \
eng_fat.c \
eng_init.c \
eng_lib.c \
eng_list.c \
eng_openssl.c \
eng_pkey.c \
eng_rdrand.c \
eng_table.c \
tb_asnmth.c \
tb_cipher.c \
tb_dh.c \
tb_digest.c \
tb_dsa.c \
tb_eckey.c \
tb_pkmeth.c \
tb_rand.c \
tb_rsa.c

SRCS += ${ENGINE_SRCS}

.if !defined(CPUID)
CRYPTOCPPFLAGS+=-DNEED_CPUID_SETUP
.endif

.for cryptosrc in ${ENGINE_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/engine ${CRYPTOCPPFLAGS} -DHAVE_CRYPTODEV
.endfor
