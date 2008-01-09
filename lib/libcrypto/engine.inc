#	$NetBSD: engine.inc,v 1.3.10.1 2008/01/09 01:34:33 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/engine/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/engine ${OPENSSLSRC}/engines


ENGINE_SRCS = eng_err.c eng_lib.c eng_list.c eng_init.c eng_ctrl.c \
	eng_table.c eng_pkey.c eng_fat.c eng_all.c \
	tb_rsa.c tb_dsa.c tb_dh.c tb_rand.c tb_cipher.c tb_digest.c \
	eng_openssl.c eng_dyn.c eng_cnf.c eng_padlock.c \
	eng_cryptodev.c tb_store.c tb_ecdsa.c tb_ecdh.c \
	e_4758cca.c e_4758cca_err.c \
	e_aep.c e_aep_err.c \
	e_atalla.c e_atalla_err.c \
	e_chil.c e_chil_err.c \
	e_cswift.c e_cswift_err.c \
	e_gmp.c e_gmp_err.c \
	e_nuron.c e_nuron_err.c \
	e_sureware.c e_sureware_err.c \
	e_ubsec.c e_ubsec_err.c 
SRCS += ${ENGINE_SRCS}

.for cryptosrc in ${ENGINE_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/engine
.endfor
