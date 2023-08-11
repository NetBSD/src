#	$NetBSD: rc4.inc,v 1.3.14.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/rc4

RC4_SRCS ?= rc4_skey.c rc4_enc.c

SRCS += ${RC4_SRCS}

.for cryptosrc in ${RC4_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rc4
.endfor
