#	$NetBSD: mdc2.inc,v 1.7 2006/03/17 23:10:48 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/mdc2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/mdc2


MDC2_SRCS = mdc2_one.c
# MDC2 is a patented algorithm; abort().
# see ../libcrypto_mdc2
MDC2_SRCS += mdc2dgst_dummy.c
#SRCS+=	mdc2dgst.c
SRCS += ${MDC2_SRCS}

.for cryptosrc in ${MDC2_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/mdc2
.endfor
