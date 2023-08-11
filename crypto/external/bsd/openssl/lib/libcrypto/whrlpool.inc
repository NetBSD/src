#	$NetBSD: whrlpool.inc,v 1.2.26.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/whrlpool

WHRLPOOL_SRCS ?= wp_block.c
WHRLPOOL_SRCS += \
wp_dgst.c \

SRCS += ${WHRLPOOL_SRCS}

.for cryptosrc in ${WHRLPOOL_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/whrlpool ${WHRLPOOLCPPFLAGS}
.endfor
