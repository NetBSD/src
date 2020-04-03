#	$NetBSD: Makefile,v 1.6 2020/04/03 17:51:02 christos Exp $

SUBDIR+= lib .WAIT usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec:
	@true

.include <bsd.subdir.mk>
