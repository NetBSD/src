#	$NetBSD: Makefile,v 1.2.2.1 2000/06/21 04:07:57 thorpej Exp $

.include <bsd.own.mk>

# RCSid:
#	$Id: Makefile,v 1.2.2.1 2000/06/21 04:07:57 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	This file is provided in the hope that it will
#	be of use.  There is absolutely NO WARRANTY.
#	Permission to copy, redistribute or otherwise
#	use this file is hereby granted provided that 
#	the above copyright notice and this notice are
#	left intact. 
#      
#	Please send copies of changes and bug-fixes to:
#	sjg@quick.com.au
#

# XXX There's a bit of work to do before we can enable warnings.
WARNS=0

PROG=	openssl

SRCS=	openssl.c
SRCS+=	verify.c asn1pars.c req.c dgst.c dh.c enc.c gendh.c gendsa.c \
	errstr.c ca.c pkcs12.c pkcs7.c pkcs8.c crl2p7.c crl.c dsa.c \
	dsaparam.c x509.c s_server.c s_client.c speed.c s_time.c apps.c \
	s_cb.c s_socket.c version.c sess_id.c ciphers.c nseq.c

LDADD+=	-lssl -lcrypto

CPPFLAGS+= -DMONOLITH -DUSE_SSL
MKMAN=	no

CRYPTODIST=	${.CURDIR}/../../crypto/dist
.include "../../crypto/Makefile.openssl"
.PATH:	${OPENSSLSRC}/apps

#LINKS= 	openssl verify \
#	openssl asn1parse \
#	openssl req \
#	openssl dgst \
#	openssl dh \
#	openssl enc \
#	openssl gendh \
#	openssl gendsa \
#	openssl errstr \
#	openssl ca \
#	openssl crl \
#	openssl dsa \
#	openssl dsaparam \
#	openssl x509 \
#	openssl s_server \
#	openssl s_client \
#	openssl sess_id \
#	openssl ciphers \
#	openssl s_filter \
#	openssl speed \
#	openssl hashdir \
#	openssl s_time \
#	openssl s_mult \
#	openssl version \
#	openssl pkcs7 \
#	openssl crl2pkcs7 \
#	openssl md2 \
#	openssl sha \
#	openssl sha1 \
#	openssl base64 \
#	openssl des \
#	openssl des3 \
#	openssl rc4 \
#	openssl rc2 \
#	openssl des-ecb \
#	openssl des-ede \
#	openssl des-ede3 \
#	openssl des-cbc \
#	openssl des-ede-cbc \
#	openssl des-ede3-cbc \
#	openssl des-cfb \
#	openssl des-ede-cfb \
#	openssl des-ede3-cfb \
#	openssl des-ofb \
#	openssl des-ede-ofb \
#	openssl des-ede3-ofb \
#	openssl rc2-cbc \
#	openssl rc2-ecb \
#	openssl rc2-cfb \
#	openssl rc2-ofb \

.if !defined(USELESS_CRYPTO)
SRCS+=	rsa.c genrsa.c 
#LINKS+=	openssl rsa \
#	openssl rsa-cbc \
#	openssl rsa-ecb \
#	openssl rsa-cfb \
#	openssl rsa-ofb
.endif

ssleay.o:	ssleay.c
	${CC} ${CFLAGS} -UMONOLITH -o ${.TARGET} -c ${.IMPSRC}

.include <bsd.prog.mk>

.if ${MKSHARE} != "no"
FILES=  CA.sh CA.pl der_chop openssl.cnf
FILESDIR=/usr/share/examples/openssl
.endif

.include <bsd.files.mk>
