#	$NetBSD: mdc2.inc,v 1.4.2.1 2002/12/01 21:36:39 he Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/mdc2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/mdc2

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/mdc2

SRCS+=	mdc2_one.c
# MDC2 is a patented algorithm; abort().
# see ../libcrypto_mdc2
SRCS+=	mdc2dgst_dummy.c
#SRCS+=	mdc2dgst.c
