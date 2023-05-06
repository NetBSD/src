#	$NetBSD: ssl.inc,v 1.11 2023/05/06 17:07:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/ssl/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/ssl

CPPFLAGS+=	-I${OPENSSLSRC}/ssl

SRCS+= \
s3_cbc.c \
bio_ssl.c \
d1_lib.c \
d1_msg.c \
d1_srtp.c \
methods.c \
pqueue.c \
s3_enc.c \
s3_lib.c \
s3_msg.c \
ssl_asn1.c \
ssl_cert.c \
ssl_ciph.c \
ssl_conf.c \
ssl_err.c \
ssl_err_legacy.c \
ssl_init.c \
ssl_lib.c \
ssl_mcnf.c \
ssl_rsa.c \
ssl_rsa_legacy.c \
ssl_sess.c \
ssl_stat.c \
ssl_txt.c \
ssl_utst.c \
t1_enc.c \
t1_lib.c \
t1_trce.c \
tls13_enc.c \
tls_depr.c \
tls_srp.c
