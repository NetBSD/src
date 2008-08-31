#	$NetBSD: Makefile,v 1.13 2008/08/31 12:09:48 christos Exp $

.include <bsd.own.mk>

MKOLDVI?=no
.if ${MKSHARE} != "no"
.if ${MKOLDVI} != "no"
SUBDIR+= catalog
.endif
SUBDIR+= recover
.endif

.if ${MKOLDVI} != "no"
SUBDIR+= build
.endif

.if make(install)
SUBDIR+= docs/USD.doc/edit docs/USD.doc/exref docs/USD.doc/vi.ref
SUBDIR+= docs/USD.doc/vitut
SUBDIR+= recover
.endif

.include <bsd.subdir.mk>
