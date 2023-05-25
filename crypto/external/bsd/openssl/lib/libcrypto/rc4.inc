#	$NetBSD: rc4.inc,v 1.5 2023/05/25 15:52:28 riastradh Exp $

.PATH:	${OPENSSLSRC}/crypto/rc4

RC4_SRCS ?= rc4_skey.c rc4_enc.c

SRCS += ${RC4_SRCS}

.for cryptosrc in ${RC4_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rc4
.endfor
