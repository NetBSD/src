#	$NetBSD: record.inc,v 1.3 2023/05/06 17:07:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/ssl/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/ssl/record

CPPFLAGS+=	-I${OPENSSLSRC}/ssl/record

SRCS+= \
tls_pad.c \
dtls1_bitmap.c \
rec_layer_d1.c \
rec_layer_s3.c \
ssl3_buffer.c \
ssl3_record.c \
ssl3_record_tls13.c
