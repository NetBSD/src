# $NetBSD: Makefile,v 1.3 2001/12/12 00:51:02 tv Exp $

DIST=		${.CURDIR}/../../crypto/dist
.PATH:		${DIST}/heimdal/lib/sl

WARNS?=		1
NOLINT=		# defined

LIB=		ss
SRCS=		sl.c ss.c
INCS=		ss.h
INCSDIR=	/usr/include/krb5

CPPFLAGS+= -I. \
	 -I${DIST}/heimdal/lib/sl		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../../lib/libcom_err	\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${.CURDIR}/../../lib/libroken	\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.include <bsd.lib.mk>
