#	$NetBSD: Makefile,v 1.3.2.2 2009/09/15 06:01:40 snj Exp $

.include <bsd.own.mk>

SUBDIR=	lib .WAIT libexec man sbin share

# The configinstall: target is invoked from the make distribution
# target in src/etc/Makefile. It is there for building the /etc/
# set. You don't want to run it normally as it would blow away your
# /etc/postfix directory.

configinstall:
	${MAKEDIRTARGET} etc configinstall

.include "Makefile.inc"

.include <bsd.subdir.mk>
