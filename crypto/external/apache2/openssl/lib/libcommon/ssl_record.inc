#	$NetBSD: ssl_record.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH:	${OPENSSLSRC}/ssl/record/methods

SSL_RECORD_SRCS += \
tls_pad.c

SRCS += ${SSL_RECORD_SRCS}

.for commonsrc in ${SSL_RECORD_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/ssl/record/methods ${SSL_RECORDCPPFLAGS}
.endfor
