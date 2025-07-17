#	$NetBSD: err.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/err

ERR_SRCS += \
err.c \
err_all.c \
err_all_legacy.c \
err_blocks.c \
err_mark.c \
err_prn.c \
err_save.c \

SRCS += ${ERR_SRCS}

.for cryptosrc in ${ERR_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/err ${ERRCPPFLAGS}
.endfor
