#	$NetBSD: crypto.inc,v 1.8.8.1 2008/06/23 04:29:36 wrstuden Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/Makefile
#

.PATH:	${OPENSSLSRC}/crypto


CRYPTO_SRCS = cryptlib.c mem.c mem_clr.c mem_dbg.c cversion.c ex_data.c \
	cpt_err.c ebcdic.c uid.c o_time.c o_dir.c
SRCS += ${CRYPTO_SRCS}

.for cryptosrc in ${CRYPTO_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/crypto
.endfor
