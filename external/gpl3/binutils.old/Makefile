#	$NetBSD: Makefile,v 1.3 2016/10/26 17:08:37 christos Exp $

SUBDIR+= lib .WAIT usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec:
	@true

.include <bsd.subdir.mk>
