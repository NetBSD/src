#	$NetBSD: property.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/property

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
