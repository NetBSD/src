#	$NetBSD: des.inc,v 1.1.1.1.2.2 2000/07/17 05:59:58 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/des/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/des

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/des

SRCS+= set_key.c  ecb_enc.c  cbc_enc.c \
	ecb3_enc.c cfb64enc.c cfb64ede.c cfb_enc.c  ofb64ede.c \
	enc_read.c enc_writ.c ofb64enc.c \
	ofb_enc.c  str2key.c  pcbc_enc.c qud_cksm.c rand_key.c \
	 des_enc.c fcrypt_b.c  read2pwd.c \
	fcrypt.c xcbc_enc.c read_pwd.c rpc_enc.c  cbc_cksm.c \
	ede_cbcm_enc.c


