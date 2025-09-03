#	$NetBSD: cmac.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/cmac

CMAC_SRCS += \
cmac.c \

SRCS += ${CMAC_SRCS}

.for cryptosrc in ${CMAC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/cmac ${CMACCPPFLAGS}
.endfor
