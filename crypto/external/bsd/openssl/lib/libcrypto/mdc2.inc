#	$NetBSD: mdc2.inc,v 1.1.8.1 2012/08/14 07:57:03 riz Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/mdc2/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/mdc2


MDC2_SRCS = mdc2_one.c
# MDC2 is a patented algorithm; abort().
# see ../libcrypto_mdc2
SRCS+=	${MDC2_SRCS} mdc2dgst.c

.for cryptosrc in ${MDC2_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/mdc2
.endfor
