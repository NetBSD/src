# $NetBSD: Makefile,v 1.1.1.1 2000/06/16 22:50:35 thorpej Exp $
DIST=		${.CURDIR}/../../dist
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

INCSDIR= /usr/include/krb5

INCS=	krb_db.h

CPPFLAGS+= -I.					\
	-I${DIST}/krb4/lib/kdb			\
	-I${DIST}/krb4/lib/krb			\
	-I${DIST}/openssl/crypto/des		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${.CURDIR}/../libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

.include <bsd.lib.mk>
