# $NetBSD: Makefile,v 1.7 2007/05/28 12:06:21 tls Exp $

USE_FORT?= yes	# network protocol library

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/sl

WARNS?=		1

LIB=		ss

LIBDPLIBS+=	edit ${.CURDIR}/../libedit \
		termcap ${.CURDIR}/../libterm \
		com_err	${.CURDIR}/../libcom_err

SRCS=		sl.c ss.c
INCS=		ss.h
INCSDIR=	/usr/include/krb5

CPPFLAGS+= -I. \
	 -I${DIST}/heimdal/lib/sl		\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${NETBSDSRCDIR}/lib/libcom_err	\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${NETBSDSRCDIR}/lib/libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.include <bsd.lib.mk>
