#	$NetBSD: err.inc,v 1.2.14.1 2023/08/11 13:41:55 martin Exp $

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
