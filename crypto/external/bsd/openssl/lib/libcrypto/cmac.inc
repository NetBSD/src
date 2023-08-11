#	$NetBSD: cmac.inc,v 1.1.56.1 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/cmac

CMAC_SRCS += \
cmac.c \

SRCS += ${CMAC_SRCS}

.for cryptosrc in ${CMAC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/cmac ${CMACCPPFLAGS}
.endfor
