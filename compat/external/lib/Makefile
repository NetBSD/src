#	$NetBSD: Makefile,v 1.2 2009/04/12 22:42:36 christos Exp $

#
# This Makefile exists to provide a single point to build
# all compat libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKATF} != "no")
SUBDIR+= ../bsd/atf/lib
.endif

SUBDIR+= ../bsd/bind/lib

.include <bsd.subdir.mk>
