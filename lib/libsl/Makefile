# $NetBSD: Makefile,v 1.6 2005/01/10 03:11:17 lukem Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/sl

LIB=		sl
SRCS=		sl.c
INCS=		sl.h
INCSDIR=	/usr/include/krb5

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/sl		\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${NETBSDSRCDIR}/lib/libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.include <bsd.lib.mk>
