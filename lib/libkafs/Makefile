# $NetBSD: Makefile,v 1.4 2001/12/12 00:51:03 tv Exp $

DIST=		${.CURDIR}/../../crypto/dist
.PATH:		${DIST}/heimdal/lib/kafs

WARNS?=		1
NOLINT=		# defined

LIB=		kafs

SRCS=		afssys.c afskrb.c afskrb5.c common.c

INCS=		kafs.h
INCSDIR=	/usr/include/kerberosIV
MAN=		kafs.3

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/kafs		\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/openssl	\
	-I${.CURDIR}/../../include/heimdal	\
	-I${.CURDIR}/../libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

.include <bsd.lib.mk>
