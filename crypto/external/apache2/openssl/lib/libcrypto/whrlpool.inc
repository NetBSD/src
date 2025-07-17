#	$NetBSD: whrlpool.inc,v 1.1 2025/07/17 14:25:27 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/whrlpool

WHRLPOOL_SRCS ?= wp_block.c
WHRLPOOL_SRCS += \
wp_dgst.c \

SRCS += ${WHRLPOOL_SRCS}

.for cryptosrc in ${WHRLPOOL_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/whrlpool ${WHRLPOOLCPPFLAGS}
.endfor
