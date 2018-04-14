#	$NetBSD: Makefile,v 1.4 2018/04/14 15:49:37 christos Exp $

SUBDIR+= lib .WAIT usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec:
	@true

.include <bsd.subdir.mk>
