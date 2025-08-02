#	$NetBSD: Makefile,v 1.3.62.1 2025/08/02 05:49:47 perseant Exp $

.include <bsd.own.mk>

SUBDIR=	lib .WAIT libexec man sbin share

# The configinstall: target is invoked from the make distribution
# target in src/etc/Makefile. It is there for building the /etc/
# set. You don't want to run it normally as it would blow away your
# /etc/postfix directory.

configinstall: .MAKE
	${MAKEDIRTARGET} etc configinstall

.include "Makefile.inc"

.include <bsd.subdir.mk>
