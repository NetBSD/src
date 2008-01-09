#	$NetBSD: crypto.inc,v 1.7.10.1 2008/01/09 01:34:30 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/Makefile
#

.PATH:	${OPENSSLSRC}/crypto


CRYPTO_SRCS = cryptlib.c mem.c mem_clr.c mem_dbg.c cversion.c ex_data.c tmdiff.c \
	cpt_err.c ebcdic.c uid.c o_time.c o_dir.c
SRCS += ${CRYPTO_SRCS}

.for cryptosrc in ${CRYPTO_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/crypto
.endfor
