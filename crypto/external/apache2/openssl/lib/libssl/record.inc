#	$NetBSD: record.inc,v 1.2 2025/07/21 22:21:21 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/ssl/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/ssl/record ${OPENSSLSRC}/ssl/record/methods

CPPFLAGS+=	-I${OPENSSLSRC}/ssl/record

SRCS+= \
dtls_meth.c \
rec_layer_d1.c \
rec_layer_s3.c \
ssl3_meth.c \
tls13_meth.c \
tls1_meth.c \
tls_common.c \
tls_multib.c \
tls_pad.c \
tlsany_meth.c \
