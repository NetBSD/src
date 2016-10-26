#	$NetBSD: Makefile,v 1.3 2016/10/26 17:09:50 christos Exp $

.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage ncdcs

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage:
	@true

.include <bsd.subdir.mk>
