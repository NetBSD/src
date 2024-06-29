#	$NetBSD: Makefile,v 1.8 2024/06/29 16:36:17 christos Exp $

.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage ncdcs

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage:
	@true

.include <bsd.subdir.mk>
