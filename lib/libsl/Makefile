# $NetBSD: Makefile,v 1.4 2001/12/12 00:51:05 tv Exp $

DIST=		${.CURDIR}/../../crypto/dist
.PATH:		${DIST}/heimdal/lib/sl

NOLINT=		# defined

LIB=		sl
SRCS=		sl.c
INCS=		sl.h
INCSDIR=	/usr/include/krb5

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/sl		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../../lib/libroken	\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.include <bsd.lib.mk>
