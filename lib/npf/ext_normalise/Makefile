# $NetBSD: Makefile,v 1.1.4.3 2012/12/13 23:50:29 riz Exp $

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
