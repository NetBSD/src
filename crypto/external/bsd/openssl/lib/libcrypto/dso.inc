#	$NetBSD: dso.inc,v 1.3.14.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/dso

DSO_SRCS += \
dso_dl.c \
dso_dlfcn.c \
dso_err.c \
dso_lib.c \
dso_openssl.c \
dso_vms.c \
dso_win32.c \

SRCS += ${DSO_SRCS}

.for cryptosrc in ${DSO_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/dso ${DSOCPPFLAGS}
.endfor
