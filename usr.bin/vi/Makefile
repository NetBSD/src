#	$NetBSD: Makefile,v 1.11 2001/05/29 21:25:23 kleink Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR+= catalog
.endif

SUBDIR+= build

.if make(install)
SUBDIR+= docs/USD.doc/edit docs/USD.doc/exref docs/USD.doc/vi.ref
SUBDIR+= docs/USD.doc/vitut
SUBDIR+= recover
.endif

.include <bsd.subdir.mk>
