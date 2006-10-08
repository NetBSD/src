#	$NetBSD: Makefile,v 1.12 2006/10/08 21:47:21 dogcow Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR+= catalog recover
.endif

SUBDIR+= build

.if make(install)
SUBDIR+= docs/USD.doc/edit docs/USD.doc/exref docs/USD.doc/vi.ref
SUBDIR+= docs/USD.doc/vitut
SUBDIR+= recover
.endif

.include <bsd.subdir.mk>
