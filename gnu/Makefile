#	$NetBSD: Makefile,v 1.11 2001/09/21 21:04:59 tv Exp $

SUBDIR+= lib .WAIT libexec usr.bin usr.sbin

# Speedup stubs for some subtrees that don't need to run these rules
includes-libexec includes-usr.bin includes-usr.sbin:
	@${TRUE}

.include <bsd.subdir.mk>
