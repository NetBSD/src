#	$NetBSD: Makefile,v 1.12.18.1 2008/09/18 04:29:25 wrstuden Exp $

.include <bsd.own.mk>

MKOLDVI?=no
.if ${MKOLDVI} != "no"

.if ${MKSHARE} != "no"
SUBDIR+= catalog recover
.endif

SUBDIR+= build

.if make(install)
SUBDIR+= docs/USD.doc/edit docs/USD.doc/exref docs/USD.doc/vi.ref
SUBDIR+= docs/USD.doc/vitut
SUBDIR+= recover
.endif

.endif

.include <bsd.subdir.mk>
