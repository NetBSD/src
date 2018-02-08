#	$NetBSD: ssl.inc,v 1.9 2018/02/08 21:57:29 christos Exp $
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
t1_ext.c \
t1_lib.c \
t1_reneg.c \
t1_trce.c \
tls_srp.c
