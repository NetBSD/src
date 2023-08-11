#	$NetBSD: property.inc,v 1.1.2.2 2023/08/11 13:41:55 martin Exp $

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
