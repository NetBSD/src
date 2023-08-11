#	$NetBSD: providers_implementations_encode_decode.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/encode_decode

PROVIDERS_IMPLEMENTATIONS_ENCODE_DECODE_SRCS += \
decode_der2key.c \
decode_epki2pki.c \
decode_msblob2key.c \
decode_pem2der.c \
decode_pvk2key.c \
decode_spki2typespki.c \
encode_key2any.c \
encode_key2blob.c \
encode_key2ms.c \
encode_key2text.c \
endecoder_common.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_ENCODE_DECODE_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_ENCODE_DECODE_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/encode_decode ${PROVIDERS_IMPLEMENTATIONS_ENCODE_DECODECPPFLAGS}
.endfor
