#	$NetBSD: Makefile,v 1.16 2009/06/23 17:58:24 tron Exp $

.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage ncdcs

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage:
	@true

.include <bsd.subdir.mk>
