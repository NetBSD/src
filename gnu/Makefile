#	$NetBSD: Makefile,v 1.13 2003/07/04 04:53:50 atatat Exp $

SUBDIR+= lib .WAIT libexec usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec includes-usr.bin:
	@true

.include <bsd.subdir.mk>
