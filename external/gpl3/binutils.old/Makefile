#	$NetBSD: Makefile,v 1.1.1.1.2.1 2016/11/04 14:43:47 pgoyette Exp $

SUBDIR+= lib .WAIT usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec:
	@true

.include <bsd.subdir.mk>
