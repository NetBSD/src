#	$NetBSD: Makefile,v 1.5 2018/04/14 17:52:51 christos Exp $

SUBDIR+= lib .WAIT usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec:
	@true

.include <bsd.subdir.mk>
