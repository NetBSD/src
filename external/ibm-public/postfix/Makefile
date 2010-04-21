#	$NetBSD: Makefile,v 1.3.4.2 2010/04/21 05:23:21 matt Exp $

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
