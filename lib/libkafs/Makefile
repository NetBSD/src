# $NetBSD: Makefile,v 1.12 2008/03/22 08:37:26 mlelstv Exp $

USE_FORT?= yes	# network protocol library

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/kafs

WARNS?=		1

LIB=		kafs

LIBDPLIBS+=	krb5	${.CURDIR}/../libkrb5 \
		roken	${.CURDIR}/../libroken

SRCS=		afssys.c afskrb.c afskrb5.c common.c

INCS=		kafs.h
INCSDIR=	/usr/include/krb5
MAN=		kafs.3

KRB5!=		cd ${NETBSDSRCDIR}/lib/libkrb5 && ${PRINTOBJDIR}

CPPFLAGS+= -I.						\
	-I${DIST}/heimdal/lib/kafs			\
	-I${KRB5}					\
	-I${NETBSDSRCDIR}/crypto/dist/heimdal/lib/krb5	\
	-I${DESTDIR}/usr/include/krb5			\
	-I${NETBSDSRCDIR}/include/heimdal		\
	-I${NETBSDSRCDIR}/libroken			\
	-I${DIST}/heimdal/lib/roken			\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.include <bsd.lib.mk>
