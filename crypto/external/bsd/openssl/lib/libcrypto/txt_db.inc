#	$NetBSD: txt_db.inc,v 1.1.58.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/txt_db

TXT_DB_SRCS += \
txt_db.c \

SRCS += ${TXT_DB_SRCS}

.for cryptosrc in ${TXT_DB_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/txt_db ${TXT_DBCPPFLAGS}
.endfor
