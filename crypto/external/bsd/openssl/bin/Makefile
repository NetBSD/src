#	$NetBSD: Makefile,v 1.8 2018/02/08 21:57:23 christos Exp $

WARNS?=	2	# XXX -Wcast-qual

NOMAN=	# defined
BINDIR=/usr/bin

.include <bsd.own.mk>

USE_FORT?= yes	# cryptographic software

# RCSid:
#	$Id: Makefile,v 1.8 2018/02/08 21:57:23 christos Exp $
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

PROG=	openssl

SRCS=	openssl.c
SRCS+=	apps.c ecparam.c ec.c pkeyparam.c genpkey.c pkey.c pkeyutl.c ts.c cms.c
SRCS+=	s_cb.c s_socket.c
SRCS+=	app_rand.c rehash.c
SRCS+=	verify.c asn1pars.c req.c dgst.c dhparam.c enc.c passwd.c errstr.c \
	opt.c ca.c \
	pkcs7.c crl2p7.c crl.c \
	rsa.c rsautl.c dsa.c dsaparam.c \
	x509.c genrsa.c gendsa.c prime.c s_server.c s_client.c speed.c \
	s_time.c version.c sess_id.c srp.c \
	ciphers.c nseq.c pkcs12.c pkcs8.c spkac.c smime.c rand.c engine.c ocsp.c

CPPFLAGS+= -DMONOLITH -DUSE_SSL -I${OPENSSLSRC}
CPPFLAGS+= -I${OPENSSLSRC}/include -I${OPENSSLSRC}/../include

LDADD+=	-lssl -lcrypto -lcrypt
DPADD+=	${LIBSSL} ${LIBCRYPTO} ${LIBCRYPT}

CRYPTODIST=	${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH:	${OPENSSLSRC}/apps

.PATH:	${OPENSSLSRC}/engines
SRCS+=	e_padlock.c

#LINKS=	openssl verify \
#	openssl asn1pars \
#	openssl req \
#	openssl dgst \
#	openssl dh \
#	openssl dhparam \
#	openssl enc \
#	openssl passwd \
#	openssl gendh \
#	openssl errstr \
#	openssl ca \
#	openssl crl \
#	openssl rsa \
#	openssl rsautl \
#	openssl dsa \
#	openssl dsaparam \
#	openssl x509 \
#	openssl genrsa \
#	openssl gendsa \
#	openssl s_server \
#	openssl s_client \
#	openssl speed \
#	openssl s_time \
#	openssl version \
#	openssl pkcs7 \
#	openssl crl2pkcs7 \
#	openssl sess_id \
#	openssl ciphers \
#	openssl nseq \
#	openssl pkcs12 \
#	openssl pkcs8 \
#	openssl spkac \
#	openssl smime \
#	openssl rand \
#	openssl engine \
#	openssl ocsp

.if ${MKSHARE} != "no"
FILES=  CA.pl openssl.cnf
FILESDIR=/usr/share/examples/openssl

CA.pl:	CA.pl.in
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_SED} \
	    -e 's@{- \$$config{hashbangperl} -}@/usr/bin/env perl@' \
	    ${.ALLSRC} > ${.TARGET}

CLEANFILES+=CA.pl

.endif

.include <bsd.prog.mk>
