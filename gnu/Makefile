#	$NetBSD: Makefile,v 1.12 2001/10/12 21:05:08 nathanw Exp $

SUBDIR+= lib .WAIT libexec usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec includes-usr.bin includes-usr.sbin:
	@true

.include <bsd.subdir.mk>
