#	$NetBSD: objects.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/objects

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
