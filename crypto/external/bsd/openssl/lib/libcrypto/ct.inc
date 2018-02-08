#	$NetBSD: ct.inc,v 1.1 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gcmsaty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/cms/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/ct


CT_SRCS = \
ct_b64.c \
ct_err.c \
ct_log.c \
ct_oct.c \
ct_policy.c \
ct_prn.c \
ct_sct.c \
ct_sct_ctx.c \
ct_vfy.c \
ct_x509v3.c

SRCS += ${CT_SRCS}

.for cryptosrc in ${CT_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ct
.endfor
