#	$NetBSD: txt_db.inc,v 1.1 2025/07/17 14:25:27 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/txt_db

TXT_DB_SRCS += \
txt_db.c \

SRCS += ${TXT_DB_SRCS}

.for cryptosrc in ${TXT_DB_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/txt_db ${TXT_DBCPPFLAGS}
.endfor
