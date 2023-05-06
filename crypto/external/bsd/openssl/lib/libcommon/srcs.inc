#	$NetBSD: srcs.inc,v 1.1 2023/05/06 17:07:22 christos Exp $

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
