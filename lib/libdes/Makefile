#	$NetBSD: Makefile,v 1.11 2003/07/24 14:16:30 itojun Exp $

.include <bsd.own.mk>

LIB=	des
SRCS=	cbc_cksm.c cbc_enc.c cfb64ede.c cfb64enc.c cfb_enc.c \
	des_enc.c ecb3_enc.c ecb_enc.c ede_cbcm_enc.c enc_read.c \
	enc_writ.c fcrypt.c fcrypt_b.c ofb64ede.c ofb64enc.c \
	ofb_enc.c pcbc_enc.c qud_cksm.c rand_key.c read2pwd.c \
	read_pwd.c rpc_enc.c set_key.c str2key.c xcbc_enc.c

# NetBSD addition
SRCS+=	rnd_keys.c

CPPFLAGS+=-DOPENSSL_VERSION_PTEXT="\" based on OpenSSL 0.9.6j 10 Apr 2003\""
CPPFLAGS+=-DOPENSSL_cleanse=bzero -DOPENSSL_malloc=malloc

INCS=	des.h
INCSDIR=/usr/include

# pod2man --section=3 --center=libdes --release=0.9.6j des.pod > des.3
# pod2man --section=7 --center=libdes --release=0.9.6j des_modes.pod > des_modes.7
MAN=	des.3 des_modes.7

MLINKS+=des.3 des_cbc_cksum.3 \
	des.3 des_cfb64_encrypt.3 \
	des.3 des_cfb_encrypt.3 \
	des.3 des_crypt.3 \
	des.3 des_ecb2_encrypt.3 \
	des.3 des_ecb3_encrypt.3 \
	des.3 des_ecb_encrypt.3 \
	des.3 des_ede2_cbc_encrypt.3 \
	des.3 des_ede2_cfb64_encrypt.3 \
	des.3 des_ede2_ofb64_encrypt.3 \
	des.3 des_ede3_cbc_encrypt.3 \
	des.3 des_ede3_cbcm_encrypt.3 \
	des.3 des_ede3_cfb64_encrypt.3 \
	des.3 des_ede3_ofb64_encrypt.3 \
	des.3 des_enc_read.3 \
	des.3 des_enc_write.3 \
	des.3 des_fcrypt.3 \
	des.3 des_is_weak_key.3 \
	des.3 des_key_sched.3 \
	des.3 des_ncbc_encrypt.3 \
	des.3 des_ofb64_encrypt.3 \
	des.3 des_ofb_encrypt.3 \
	des.3 des_pcbc_encrypt.3 \
	des.3 des_quad_cksum.3 \
	des.3 des_random_key.3 \
	des.3 des_read_2passwords.3 \
	des.3 des_read_password.3 \
	des.3 des_read_pw_string.3 \
	des.3 des_set_key.3 \
	des.3 des_set_key_checked.3 \
	des.3 des_set_key_unchecked.3 \
	des.3 des_set_odd_parity.3 \
	des.3 des_string_to_2keys.3 \
	des.3 des_string_to_key.3 \
	des.3 des_xcbc_encrypt.3

.include <bsd.lib.mk>
