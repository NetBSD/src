#	$NetBSD: Makefile,v 1.1 2009/01/19 07:19:09 jmmv Exp $

#
# This Makefile exists to provide a single point to build
# all compat libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKATF} != "no")
SUBDIR+= ../bsd/atf/lib
.endif

.include <bsd.subdir.mk>
