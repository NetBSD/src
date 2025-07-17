#	$NetBSD: cmp.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/cmp

CMP_SRCS += \
cmp_asn.c \
cmp_client.c \
cmp_ctx.c \
cmp_err.c \
cmp_genm.c \
cmp_hdr.c \
cmp_http.c \
cmp_msg.c \
cmp_protect.c \
cmp_server.c \
cmp_status.c \
cmp_util.c \
cmp_vfy.c \

SRCS += ${CMP_SRCS}

.for cryptosrc in ${CMP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/cmp ${CMPCPPFLAGS}
.endfor
