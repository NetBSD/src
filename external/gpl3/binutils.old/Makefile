#	$NetBSD: Makefile,v 1.1.1.1 2016/01/26 17:25:55 christos Exp $

SUBDIR+= lib .WAIT usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec:
	@true

.include <bsd.subdir.mk>
