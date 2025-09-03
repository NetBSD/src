#	$NetBSD: objects.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/objects

OBJECTS_SRCS += \
o_names.c \
obj_dat.c \
obj_err.c \
obj_lib.c \
obj_xref.c \

SRCS += ${OBJECTS_SRCS}

.for cryptosrc in ${OBJECTS_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/objects ${OBJECTSCPPFLAGS}
.endfor
