# $NetBSD: Makefile,v 1.6 2002/08/19 13:35:08 lukem Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/krb4/lib/kdb

WARNS?=		1

LIB=		kdb

SRCS=		krb_cache.c krb_kdb_utils.c copykey.c \
		krb_lib.c krb_dbm.c print_princ.c

INCS=		krb_db.h
INCSDIR=	/usr/include/kerberosIV

LIBKRBINC!=	cd ${NETBSDSRCDIR}/lib/libkrb && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	-I${DIST}/krb4/lib/kdb			\
	-I${LIBKRBINC}				\
	-I${DIST}/krb4/lib/krb			\
	-I${DIST}/openssl/crypto/des		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${NETBSDSRCDIR}/lib/libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

.include <bsd.lib.mk>
