#	$NetBSD: des.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/des/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/des

CFLAGS+=	-I${OPENSSLSRC}/crypto/des

SRCS+= set_key.c  ecb_enc.c  cbc_enc.c \
	ecb3_enc.c cfb64enc.c cfb64ede.c cfb_enc.c  ofb64ede.c \
	enc_read.c enc_writ.c ofb64enc.c \
	ofb_enc.c  str2key.c  pcbc_enc.c qud_cksm.c rand_key.c \
	 des_enc.c fcrypt_b.c  read2pwd.c \
	fcrypt.c xcbc_enc.c read_pwd.c rpc_enc.c  cbc_cksm.c supp.c \
	ede_cbcm_enc.c


