#	$NetBSD: Makefile,v 1.10 2001/04/11 21:01:44 aymeric Exp $

SUBDIR= catalog build

.if make(install)
SUBDIR+= docs/USD.doc/edit docs/USD.doc/exref docs/USD.doc/vi.ref
SUBDIR+= docs/USD.doc/vitut
SUBDIR+= recover
.endif

.include <bsd.subdir.mk>
