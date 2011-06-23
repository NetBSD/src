#	$NetBSD: crypto.inc,v 1.1.4.1 2011/06/23 14:18:09 cherry Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/Makefile
#

.PATH:	${OPENSSLSRC}/crypto


CRYPTO_SRCS = cryptlib.c mem.c mem_clr.c mem_dbg.c cversion.c ex_data.c \
	cpt_err.c ebcdic.c uid.c o_time.c o_str.c o_dir.c o_fips.c o_init.c
SRCS += ${CRYPTO_SRCS}

.for cryptosrc in ${CRYPTO_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/crypto
.endfor
