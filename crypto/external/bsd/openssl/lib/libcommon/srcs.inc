#	$NetBSD: srcs.inc,v 1.1.2.2 2023/08/11 13:41:54 martin Exp $

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
