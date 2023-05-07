#	$NetBSD: Makefile,v 1.11 2023/05/07 16:19:24 christos Exp $

WARNS?=	2	# XXX -Wcast-qual

NOMAN=	# defined
BINDIR=/usr/bin

.include <bsd.own.mk>

USE_FORT?= yes	# cryptographic software

# RCSid:
#	$Id: Makefile,v 1.11 2023/05/07 16:19:24 christos Exp $
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

SRCS+= \
asn1parse.c \
ca.c \
ciphers.c \
cmp.c \
cms.c \
crl.c \
crl2pkcs7.c \
dgst.c \
dhparam.c \
dsa.c \
dsaparam.c \
ec.c \
ecparam.c \
enc.c \
engine.c \
errstr.c \
fipsinstall.c \
gendsa.c \
genpkey.c \
genrsa.c \
info.c \
kdf.c \
list.c \
mac.c \
nseq.c \
ocsp.c \
openssl.c \
passwd.c \
pkcs12.c \
pkcs7.c \
pkcs8.c \
pkey.c \
pkeyparam.c \
pkeyutl.c \
prime.c \
progs.c \
rand.c \
rehash.c \
req.c \
rsa.c \
rsautl.c \
s_client.c \
s_server.c \
s_time.c \
sess_id.c \
smime.c \
speed.c \
spkac.c \
srp.c \
storeutl.c \
ts.c \
verify.c \
version.c \
x509.c \

CPPFLAGS+= -DMONOLITH -DUSE_SSL -I${OPENSSLSRC} -I${.CURDIR}
CPPFLAGS+= -I${OPENSSLSRC}/include -I${OPENSSLSRC}/apps/include

PROGDPLIBSSTATIC=yes
PROGDPLIBS+=    apps ${OPENSSLSRC}/../lib/libapps
PROGDPLIBS+=    ssl ${OPENSSLSRC}/../lib/libssl
PROGDPLIBS+=    crypto ${OPENSSLSRC}/../lib/libcrypto

LDADD+=	-lcrypt
DPADD+=	${LIBCRYPT}

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
FILESBUILD= yes
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
