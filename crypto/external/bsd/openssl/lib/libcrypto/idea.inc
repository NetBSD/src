#	$NetBSD: idea.inc,v 1.4.20.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/idea

IDEA_SRCS += \
i_cbc.c \
i_cfb64.c \
i_ecb.c \
i_ofb64.c \
i_skey.c \

SRCS += ${IDEA_SRCS}

.for cryptosrc in ${IDEA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/idea ${IDEACPPFLAGS}
.endfor
