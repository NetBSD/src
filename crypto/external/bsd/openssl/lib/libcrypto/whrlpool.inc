#	$NetBSD: whrlpool.inc,v 1.3 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/whrlpool

WHRLPOOL_SRCS += \
wp_dgst.c \

SRCS += ${WHRLPOOL_SRCS}

.for cryptosrc in ${WHRLPOOL_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/whrlpool ${WHRLPOOLCPPFLAGS}
.endfor
