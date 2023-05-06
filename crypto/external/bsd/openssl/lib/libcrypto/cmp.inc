#	$NetBSD: cmp.inc,v 1.1 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/cmp

CMP_SRCS += \
cmp_asn.c \
cmp_client.c \
cmp_ctx.c \
cmp_err.c \
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
