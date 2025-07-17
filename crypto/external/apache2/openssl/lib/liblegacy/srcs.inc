#	$NetBSD: srcs.inc,v 1.1 2025/07/17 14:26:12 christos Exp $


#CRYPTOINCS= \
#md5.inc \
#rc4.inc \

LEGACYINCS= \
crypto_des.inc \
crypto_rc4.inc \
crypto_md5.inc \
providers.inc \
providers_common.inc \
providers_implementations_ciphers.inc \
providers_implementations_digests.inc \
providers_implementations_kdfs.inc \

.if exists(${.CURDIR}/arch/${MACHINE_ARCH})
CRYPTO_MACHINE_CPU?=    ${MACHINE_ARCH}
.else
CRYPTO_MACHINE_CPU?=    ${MACHINE_CPU}
.endif

.for cryptoinc in ${CRYPTOINCS}
.if exists(${.CURDIR}/../libcrypto/arch/${CRYPTO_MACHINE_CPU}/${cryptoinc})
.include "${.CURDIR}/../libcrypto/arch/${CRYPTO_MACHINE_CPU}/${cryptoinc}"
.else
.include "${.CURDIR}/../libcrypto/${cryptoinc}"
.endif
.endfor

.for legacyinc in ${LEGACYINCS}
.if exists(${.CURDIR}/arch/${LEGACY_MACHINE_CPU}/${legacyinc})
.include "${.CURDIR}/arch/${LEGACY_MACHINE_CPU}/${legacyinc}"
.else
.include "${legacyinc}"
.endif
.endfor
