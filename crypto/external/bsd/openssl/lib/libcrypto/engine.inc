#	$NetBSD: engine.inc,v 1.14 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/engine

ENGINE_SRCS += \
eng_all.c \
eng_cnf.c \
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
tb_rsa.c \

SRCS += ${ENGINE_SRCS}

.if !defined(CPUID)
CRYPTOCPPFLAGS+=-DNEED_CPUID_SETUP
.endif

.for cryptosrc in ${ENGINE_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/engine ${ENGINECPPFLAGS}
.endfor
