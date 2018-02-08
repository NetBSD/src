#	$NetBSD: record.inc,v 1.1 2018/02/08 21:57:29 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/ssl/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/ssl/record

CPPFLAGS+=	-I${OPENSSLSRC}/ssl/record

SRCS+= \
dtls1_bitmap.c \
rec_layer_d1.c \
rec_layer_s3.c \
ssl3_buffer.c \
ssl3_record.c \
