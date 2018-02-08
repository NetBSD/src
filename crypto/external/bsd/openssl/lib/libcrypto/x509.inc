#	$NetBSD: x509.inc,v 1.2 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/x509/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/x509


X509_SRCS = \
by_dir.c \
by_file.c \
t_crl.c \
t_req.c \
t_x509.c \
x509_att.c \
x509_cmp.c \
x509_d2.c \
x509_def.c \
x509_err.c \
x509_ext.c \
x509_lu.c \
x509_obj.c \
x509_r2x.c \
x509_req.c \
x509_set.c \
x509_trs.c \
x509_txt.c \
x509_v3.c \
x509_vfy.c \
x509_vpm.c \
x509cset.c \
x509name.c \
x509rset.c \
x509spki.c \
x509type.c \
x_all.c \
x_attrib.c \
x_crl.c \
x_exten.c \
x_name.c \
x_pubkey.c \
x_req.c \
x_x509.c \
x_x509a.c

SRCS += ${X509_SRCS}

.for cryptosrc in ${X509_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/x509
.endfor
