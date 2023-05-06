#	$NetBSD: srcs.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

DEFAULTINCS= \
providers_common.inc \
providers_common_der.inc \
providers_implementations_asymciphers.inc \
providers_implementations_ciphers.inc \
providers_implementations_digests.inc \
providers_implementations_encode_decode.inc \
providers_implementations_exchange.inc \
providers_implementations_kdfs.inc \
providers_implementations_kem.inc \
providers_implementations_keymgmt.inc \
providers_implementations_macs.inc \
providers_implementations_rands.inc \
providers_implementations_rands_seeding.inc \
providers_implementations_signature.inc \
providers_implementations_storemgmt.inc \
ssl.inc \

.for defaultinc in ${DEFAULTINCS}
.if exists(${.CURDIR}/arch/${DEFAULT_MACHINE_CPU}/${defaultinc})
.include "${.CURDIR}/arch/${DEFAULT_MACHINE_CPU}/${defaultinc}"
.else
.include "${defaultinc}"
.endif
.endfor
