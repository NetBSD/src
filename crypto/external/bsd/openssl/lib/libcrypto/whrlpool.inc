#	$NetBSD: whrlpool.inc,v 1.4 2023/05/11 20:08:59 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/whrlpool

WHRLPOOL_SRCS ?= wp_block.c
WHRLPOOL_SRCS += \
wp_dgst.c \

SRCS += ${WHRLPOOL_SRCS}

.for cryptosrc in ${WHRLPOOL_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/whrlpool ${WHRLPOOLCPPFLAGS}
.endfor
