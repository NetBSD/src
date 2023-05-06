#	$NetBSD: idea.inc,v 1.5 2023/05/06 17:07:23 christos Exp $

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
