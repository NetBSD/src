# $NetBSD: Makefile,v 1.5 2002/08/19 09:41:31 lukem Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/kafs

WARNS?=		1

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
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${NETBSDSRCDIR}/libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

.include <bsd.lib.mk>
