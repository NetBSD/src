#	$NetBSD: Makefile,v 1.1.1.1.2.1 2016/11/04 14:45:10 pgoyette Exp $

.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage ncdcs

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage:
	@true

.include <bsd.subdir.mk>
