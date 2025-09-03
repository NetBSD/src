#	$NetBSD: ts.inc,v 1.1 2025/07/17 14:25:27 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/ts

TS_SRCS += \
ts_asn1.c \
ts_conf.c \
ts_err.c \
ts_lib.c \
ts_req_print.c \
ts_req_utils.c \
ts_rsp_print.c \
ts_rsp_sign.c \
ts_rsp_utils.c \
ts_rsp_verify.c \
ts_verify_ctx.c \

SRCS += ${TS_SRCS}

.for cryptosrc in ${TS_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ts ${TSCPPFLAGS}
.endfor
