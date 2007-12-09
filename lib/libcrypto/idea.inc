#	$NetBSD: idea.inc,v 1.7 2007/12/09 22:44:17 adrianp Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/idea/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/idea


IDEA_SRCS = i_cfb64.c i_ofb64.c i_ecb.c
# IDEA is a patented algorithm; abort().
# see ../libcrypto_idea
IDEA_SRCS += i_cbc_dummy.c i_skey_dummy.c
#SRCS+=i_cbc.c i_skey.c
SRCS += ${IDEA_SRCS}

.for cryptosrc in ${IDEA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/idea
.endfor
