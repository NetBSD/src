# $NetBSD: Makefile,v 1.5 2002/08/19 09:41:34 lukem Exp $

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

.include <bsd.lib.mk>
