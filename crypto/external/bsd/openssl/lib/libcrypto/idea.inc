#	$NetBSD: idea.inc,v 1.1.8.1.6.1 2015/04/02 18:04:14 snj Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/idea/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/idea


IDEA_SRCS = i_cfb64.c i_ofb64.c i_ecb.c
# IDEA is a patented algorithm; abort().
# see ../libcrypto_idea
SRCS+=${IDEA_SRCS} i_cbc.c i_skey.c

.for cryptosrc in ${IDEA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/idea
.endfor
