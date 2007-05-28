# $NetBSD: Makefile,v 1.3 2007/05/28 12:06:37 tls Exp $

USE_FORT?=yes	# network client

PROG=	mld6query
SRCS=	mld6.c
MAN=	mld6query.8

CPPFLAGS+= -DINET6 -DIPSEC

.include <bsd.prog.mk>
