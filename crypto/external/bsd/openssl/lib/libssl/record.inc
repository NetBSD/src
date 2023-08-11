#	$NetBSD: record.inc,v 1.2.10.1 2023/08/11 13:42:15 martin Exp $
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
