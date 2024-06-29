#	$NetBSD: Makefile,v 1.8 2024/06/29 16:35:40 christos Exp $

SUBDIR+= lib .WAIT usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec:
	@true

.include <bsd.subdir.mk>
