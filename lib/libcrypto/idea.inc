#	$NetBSD: idea.inc,v 1.5 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /users/itojun/NetBSD/basesrc/lib/libcrypto/../../crypto/dist/openssl/crypto/idea/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/idea


IDEA_SRCS = i_cfb64.c i_ofb64.c i_ecb.c
# IDEA is a patented algorithm; abort().
# see ../libcrypto_idea
IDEA_SRCS = i_cbc_dummy.c i_skey_dummy.c
#SRCS+=i_cbc.c i_skey.c
SRCS += ${IDEA_SRCS}

.for cryptosrc in ${IDEA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/idea
.endfor
