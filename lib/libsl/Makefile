# $NetBSD: Makefile,v 1.9 2010/02/03 15:34:43 roy Exp $

USE_FORT?= yes	# network protocol library

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/sl

LIB=		sl

LIBDPLIBS+=	edit ${.CURDIR}/../libedit \
		terminfo ${.CURDIR}/../libterminfo

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
