#	$NetBSD: crypto.inc,v 1.7 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto


CRYPTO_SRCS = cryptlib.c mem.c mem_clr.c mem_dbg.c cversion.c ex_data.c tmdiff.c \
	cpt_err.c ebcdic.c uid.c o_time.c o_dir.c
SRCS += ${CRYPTO_SRCS}

.for cryptosrc in ${CRYPTO_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/crypto
.endfor
