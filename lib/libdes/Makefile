#	$NetBSD: Makefile,v 1.9 2003/07/23 05:46:00 itojun Exp $

.include <bsd.own.mk>

LIB=	des
SRCS=	cbc_cksm.c cbc_enc.c cfb64ede.c cfb64enc.c cfb_enc.c \
	des_enc.c ecb3_enc.c ecb_enc.c ede_cbcm_enc.c enc_read.c \
	enc_writ.c fcrypt.c fcrypt_b.c ofb64ede.c ofb64enc.c \
	ofb_enc.c pcbc_enc.c qud_cksm.c rand_key.c read2pwd.c \
	read_pwd.c rpc_enc.c set_key.c str2key.c xcbc_enc.c
SRCS+=	rnd_keys.c

CPPFLAGS+=-DOPENSSL_VERSION_PTEXT="\" based on OpenSSL 0.9.6j 10 Apr 2003\""
CPPFLAGS+=-DOPENSSL_cleanse=bzero -DOPENSSL_malloc=malloc

INCS=	des.h
INCSDIR=/usr/include

# pod2man --section=3 --center=libdes --release=0.9.6j des.pod > des.3
# pod2man --section=7 --center=libdes --release=0.9.6j des_modes.pod > des_modes.7
MAN=	des.3 des_modes.7

.include <bsd.lib.mk>
