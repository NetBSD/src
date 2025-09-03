#	$NetBSD: property.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/property

PROPERTY_SRCS += \
defn_cache.c \
property.c \
property_err.c \
property_parse.c \
property_query.c \
property_string.c \

SRCS += ${PROPERTY_SRCS}

.for cryptosrc in ${PROPERTY_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/property ${PROPERTYCPPFLAGS}
.endfor
