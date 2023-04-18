#	$NetBSD: kdf.inc,v 1.1.1.1 2023/04/18 14:19:04 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gkdfaty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/kdf/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/kdf


KDF_SRCS = \
hkdf.c \
kdf_err.c \
tls1_prf.c

SRCS += ${KDF_SRCS}

.for cryptosrc in ${KDF_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/kdf
.endfor
