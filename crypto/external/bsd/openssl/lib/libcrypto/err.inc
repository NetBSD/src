#	$NetBSD: err.inc,v 1.3 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/err

ERR_SRCS += \
err.c \
err_all.c \
err_all_legacy.c \
err_blocks.c \
err_prn.c \

SRCS += ${ERR_SRCS}

.for cryptosrc in ${ERR_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/err ${ERRCPPFLAGS}
.endfor
