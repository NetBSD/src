#	$NetBSD: Makefile,v 1.3.12.1 2018/04/16 01:58:46 pgoyette Exp $

SUBDIR+= lib .WAIT usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec:
	@true

.include <bsd.subdir.mk>
