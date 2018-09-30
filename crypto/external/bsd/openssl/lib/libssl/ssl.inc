#	$NetBSD: ssl.inc,v 1.9.2.1 2018/09/30 01:45:00 pgoyette Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/ssl/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/ssl

CPPFLAGS+=	-I${OPENSSLSRC}/ssl

SRCS+= \
bio_ssl.c \
d1_lib.c \
d1_msg.c \
d1_srtp.c \
methods.c \
packet.c \
pqueue.c \
s3_cbc.c \
s3_enc.c \
s3_lib.c \
s3_msg.c \
ssl_asn1.c \
ssl_cert.c \
ssl_ciph.c \
ssl_conf.c \
ssl_err.c \
ssl_init.c \
ssl_lib.c \
ssl_mcnf.c \
ssl_rsa.c \
ssl_sess.c \
ssl_stat.c \
ssl_txt.c \
ssl_utst.c \
t1_enc.c \
t1_lib.c \
t1_trce.c \
tls_srp.c \
tls13_enc.c
