#	$NetBSD: Makefile,v 1.2 2001/08/14 11:40:40 tv Exp $

.include <bsd.own.mk>

# XXX - Temporarily disable building locale files on USE_NEW_TOOLCHAIN. - XXX
#
# The /usr/bin/mklocale program is very tied to NetBSD-current and is useless
# for cross builds (as is one of the main purposes of the new toolchain).
# Disable building the ctype files until mklocale is in a cross-usable state.

.ifndef USE_NEW_TOOLCHAIN
SUBDIR=	ctype
.endif

.include <bsd.subdir.mk>
