#	$NetBSD: txt_db.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/txt_db

TXT_DB_SRCS += \
txt_db.c \

SRCS += ${TXT_DB_SRCS}

.for cryptosrc in ${TXT_DB_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/txt_db ${TXT_DBCPPFLAGS}
.endfor
