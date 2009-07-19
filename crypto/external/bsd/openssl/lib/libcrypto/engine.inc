#	$NetBSD: engine.inc,v 1.1 2009/07/19 23:30:43 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/engine/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/engine ${OPENSSLSRC}/engines \
    ${OPENSSLSRC}/engines/ccgost


ENGINE_SRCS = eng_err.c eng_lib.c eng_list.c eng_init.c eng_ctrl.c \
	eng_table.c eng_pkey.c eng_fat.c eng_all.c tb_asnmth.c \
	tb_rsa.c tb_dsa.c tb_dh.c tb_rand.c tb_cipher.c tb_digest.c \
	eng_openssl.c eng_dyn.c eng_cnf.c \
	eng_cryptodev.c tb_store.c tb_ecdsa.c tb_ecdh.c tb_pkmeth.c \
	e_4758cca.c e_4758cca_err.c \
	e_aep.c e_aep_err.c \
	e_atalla.c e_atalla_err.c \
	e_chil.c e_chil_err.c \
	e_cswift.c e_cswift_err.c \
	e_gmp.c e_gmp_err.c \
	e_nuron.c e_nuron_err.c \
	e_sureware.c e_sureware_err.c \
	e_ubsec.c e_ubsec_err.c   \

.if defined(AESNI)
ENGINE_SRCS+=	eng_aesni.c
.else
AESCPPFLAGS=-DOPENSSL_NO_HW_AESNI
.endif

#ENGINE_SRCS += gost2001.c gost2001_keyx.c gost89.c gost94_keyx.c gost_ameth.c \
#	gost_asn1.c gost_crypt.c gost_ctl.c gost_eng.c gosthash.c \
#	gost_keywrap.c gost_md.c gost_params.c gost_pmeth.c gost_sign.c

SRCS += ${ENGINE_SRCS}

.for cryptosrc in ${ENGINE_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/engine ${AESCPPFLAGS}
.endfor
