#	$NetBSD: x509.inc,v 1.1.1.1 2000/06/16 04:21:31 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/x509/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/x509

CFLAGS+=	-I${OPENSSLSRC}/crypto/x509

SRCS+= x509_def.c x509_d2.c x509_r2x.c x509_cmp.c \
	x509_obj.c x509_req.c x509_vfy.c \
	x509_set.c x509rset.c x509_err.c \
	x509name.c x509_v3.c x509_ext.c \
	x509type.c x509_lu.c x_all.c x509_txt.c \
	by_file.c by_dir.c


