#	$NetBSD: Makefile,v 1.1 2008/05/20 18:04:16 aymeric Exp $

.include "Makefile.inc"

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR+= catalog
.endif

SUBDIR+= build

#
# Don't descend into docs/ and recover/ for now, we leave the old versions.
# Recover should be fine for our two versions of nvi.
# I will take care of the documentation later.
# 
#.if make(install)
#SUBDIR+= docs/USD.doc/edit docs/USD.doc/exref docs/USD.doc/vi.ref
#SUBDIR+= docs/USD.doc/vitut
#SUBDIR+= recover
#.endif

.include <bsd.subdir.mk>
