# $NetBSD: Makefile,v 1.3 2001/09/17 12:34:42 assar Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/krb4/lib/kdb

WARNS?=  1
MKLINT= no

LIB=	kdb

SRCS=	krb_cache.c				\
	krb_kdb_utils.c				\
	copykey.c				\
	krb_lib.c				\
	krb_dbm.c				\
	print_princ.c

INCSDIR= /usr/include/kerberosIV

INCS=	krb_db.h

LIBKRBINC!=	cd ${.CURDIR}/../libkrb && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	-I${DIST}/krb4/lib/kdb			\
	-I${LIBKRBINC}				\
	-I${DIST}/krb4/lib/krb			\
	-I${DIST}/openssl/crypto/des		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${.CURDIR}/../libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

.include <bsd.lib.mk>
