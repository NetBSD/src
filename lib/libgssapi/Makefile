# $NetBSD: Makefile,v 1.4 2000/12/30 17:21:44 sommerfeld Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/gssapi

WARNS?=  1
MKLINT= no

LIB=	gssapi

SRCS	 =		\
	8003.c			\
	accept_sec_context.c	\
	acquire_cred.c		\
	add_oid_set_member.c	\
	canonicalize_name.c	\
	compare_name.c		\
	context_time.c		\
	create_emtpy_oid_set.c	\
	decapsulate.c		\
	delete_sec_context.c	\
	display_name.c		\
	display_status.c	\
	duplicate_name.c	\
	encapsulate.c		\
	export_name.c		\
	export_sec_context.c	\
	external.c		\
	get_mic.c		\
	gssapi.h		\
	gssapi_locl.h		\
	import_name.c		\
	import_sec_context.c	\
	indicate_mechs.c	\
	init.c			\
	init_sec_context.c	\
	inquire_context.c	\
	inquire_cred.c		\
	release_buffer.c	\
	release_cred.c		\
	release_name.c		\
	release_oid_set.c	\
	test_oid_set_member.c	\
	unwrap.c		\
	v1.c			\
	verify_mic.c		\
	wrap.c			\
	address_to_krb5addr.c

INCSDIR= /usr/include/gssapi

INCS=	gssapi.h

ASN1!=	cd ${.CURDIR}/../libasn1 && ${PRINTOBJDIR}
KRB5!=	cd ${.CURDIR}/../libkrb5 && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/gssapi		\
	 -I${ASN1}				\
	 -I${KRB5}				\
	 -I${DIST}/heimdal/lib/krb5		\
	 -I${DIST}/openssl/crypto/md4		\
	 -I${DIST}/openssl/crypto/md5		\
	 -I${DIST}/openssl/crypto/sha		\
	 -I${DIST}/openssl/crypto/rc4		\
	 -I${DIST}/openssl/crypto/des		\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.include <bsd.lib.mk>
