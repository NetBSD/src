#	$NetBSD: rc5.inc,v 1.5 2003/07/24 14:16:33 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /users/itojun/NetBSD/basesrc/lib/libcrypto/../../crypto/dist/openssl/crypto/rc5/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc5

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rc5

SRCS+=rc5_ecb.c rc5cfb64.c rc5ofb64.c
# RC5 is a patented algorithm; abort().
# see ../libcrypto_rc5
SRCS+=rc5_skey_dummy.c rc5_enc_dummy.c
#SRCS+=rc5_skey.c rc5_enc.c
