#	$NetBSD: des.inc,v 1.2 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/des/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/des

DES_SRCS ?= des_enc.c fcrypt_b.c
DES_SRCS += \
cbc_cksm.c \
cbc_enc.c \
cfb64ede.c \
cfb64enc.c \
cfb_enc.c \
ecb3_enc.c \
ecb_enc.c \
fcrypt.c \
ofb64ede.c \
ofb64enc.c \
ofb_enc.c \
pcbc_enc.c \
qud_cksm.c \
rand_key.c \
rpc_enc.c \
set_key.c \
str2key.c \
xcbc_enc.c

SRCS += ${DES_SRCS}

.for cryptosrc in ${DES_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/des
.endfor
