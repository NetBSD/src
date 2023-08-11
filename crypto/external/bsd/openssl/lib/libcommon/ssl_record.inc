#	$NetBSD: ssl_record.inc,v 1.1.2.2 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/ssl/record

SSL_RECORD_SRCS += \
tls_pad.c \

SRCS += ${SSL_RECORD_SRCS}

.for commonsrc in ${SSL_RECORD_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/ssl/record ${SSL_RECORDCPPFLAGS}
.endfor
