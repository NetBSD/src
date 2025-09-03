#	$NetBSD: srcs.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

COMMONINCS= \
providers_common.inc \
providers_common_der.inc \
providers_implementations_ciphers.inc \
providers_implementations_digests.inc \
ssl_record.inc \

.for commoninc in ${COMMONINCS}
.if exists(${.CURDIR}/arch/${COMMON_MACHINE_CPU}/${commoninc})
.include "${.CURDIR}/arch/${COMMON_MACHINE_CPU}/${commoninc}"
.else
.include "${commoninc}"
.endif
.endfor
