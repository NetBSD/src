# $NetBSD: Makefile,v 1.7 2003/07/23 08:01:43 itojun Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/kafs

WARNS?=		1

LIB=		kafs

SRCS=		afssys.c afskrb.c afskrb5.c common.c

INCS=		kafs.h
INCSDIR=	/usr/include/krb5
MAN=		kafs.3

.if (${MKKERBEROS4} != "no")
CPPFLAGS+=	-DKRB4
CPPFLAGS+=	-I${DESTDIR}/usr/include/kerberosIV
.else
CPPFLAGS+=	-I${NETBSDSRCDIR}/crypto/dist/heimdal/lib/krb5
.endif

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/kafs		\
	-I${DESTDIR}/usr/include/krb5		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${NETBSDSRCDIR}/libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H
.if ${MKKERBEROS4} != "no"
CPPFLAGS+=-DKRB4
.endif

.include <bsd.lib.mk>
