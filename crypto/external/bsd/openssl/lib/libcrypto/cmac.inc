#	$NetBSD: cmac.inc,v 1.2 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/cmac

CMAC_SRCS += \
cmac.c \

SRCS += ${CMAC_SRCS}

.for cryptosrc in ${CMAC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/cmac ${CMACCPPFLAGS}
.endfor
