# $NetBSD: Makefile,v 1.2 2012/12/09 22:06:21 spz Exp $

.include <bsd.own.mk>

LIBISMODULE= yes
.if defined(MLIBDIR)
LIBDIR=         /usr/lib/${MLIBDIR}/npf
SHLIBDIR=       /usr/lib/${MLIBDIR}/npf
SHLIBINSTALLDIR=/usr/lib/${MLIBDIR}/npf
.else
LIBDIR=         /usr/lib/npf
SHLIBDIR=       /usr/lib/npf
SHLIBINSTALLDIR=/usr/lib/npf
.endif

LIB=	ext_normalise

SRCS=	npfext_normalise.c
WARNS=	5

.include <bsd.lib.mk>
