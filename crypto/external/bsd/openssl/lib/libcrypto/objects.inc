#	$NetBSD: objects.inc,v 1.1.58.1 2023/08/11 13:41:55 martin Exp $

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
