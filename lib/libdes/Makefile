#	$NetBSD: Makefile,v 1.1.1.1 2000/06/17 01:08:24 thorpej Exp $

LIB=	des

#.if (${MACHINE_ARCH} == "i386")
#DES_ENC= asm/dx86-elf.cpp asm/yx86-elf.cpp
#.else
DES_ENC= des_enc.c fcrypt_b.c
#.endif

SRCS=	cbc_cksm.c cbc_enc.c  cfb64enc.c cfb_enc.c  \
	ecb3_enc.c ecb_enc.c  enc_read.c enc_writ.c \
	fcrypt.c ofb64enc.c ofb_enc.c  pcbc_enc.c \
	qud_cksm.c rand_key.c read_pwd.c rpc_enc.c  set_key.c  \
	$(DES_ENC) read2pwd.c \
	fcrypt.c xcbc_enc.c \
	str2key.c  cfb64ede.c ofb64ede.c supp.c ede_cbcm_enc.c

CPPFLAGS+=	-D_POSIX_SOURCE -I. -I${.CURDIR}
CPPFLAGS+=      -I${DIST}/openssl/crypto/des -I${DIST}/openssl/crypto

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/openssl/crypto/des ${DIST}/openssl/crypto ${DIST}/openssl

INCS=	des.h 
INCSDIR=/usr/include/openssl

.include <bsd.lib.mk>
