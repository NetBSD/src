#	$NetBSD: des.inc,v 1.1 2001/09/09 10:44:24 tls Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#

.PATH:	${OPENSSLSRC}/crypto/des
.PATH:	${.CURDIR}/arch/i386

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/des
CPPFLAGS+=	-I${.CURDIR}/arch/i386

SRCS+=	cbc_cksm.c cbc_enc.c  cfb64enc.c cfb_enc.c  \
	ecb3_enc.c ecb_enc.c  enc_read.c enc_writ.c \
	fcrypt.c ofb64enc.c ofb_enc.c  pcbc_enc.c \
	qud_cksm.c rand_key.c read_pwd.c rpc_enc.c  set_key.c  \
	des_enc.S des_enc3.c fcrypt_b.c read2pwd.c \
	xcbc_enc.c \
	str2key.c  cfb64ede.c ofb64ede.c ede_cbcm_enc.c
