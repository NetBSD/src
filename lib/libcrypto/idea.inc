#	$NetBSD: idea.inc,v 1.2.2.2 2000/10/07 01:14:16 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /users/itojun/NetBSD/basesrc/lib/libcrypto/../../crypto/dist/openssl/crypto/idea/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/idea

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/idea

SRCS+=i_cfb64.c i_ofb64.c i_ecb.c
# IDEA is a patented algorithm; abort().
# see ../libcrypto_idea
SRCS+=i_cbc_dummy.c i_skey_dummy.c
#SRCS+=i_cbc.c i_skey.c
