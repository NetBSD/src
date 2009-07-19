#	$NetBSD: mdc2.inc,v 1.1 2009/07/19 23:30:44 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/mdc2/Makefile
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
