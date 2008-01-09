#	$NetBSD: pem.inc,v 1.5.10.1 2008/01/09 01:34:37 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/pem/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/pem


PEM_SRCS = pem_sign.c pem_seal.c pem_info.c pem_lib.c pem_all.c pem_err.c \
	pem_x509.c pem_xaux.c pem_oth.c pem_pk8.c pem_pkey.c
SRCS += ${PEM_SRCS}

.for cryptosrc in ${PEM_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/pem
.endfor
