#	$NetBSD: Makefile,v 1.2 2008/09/21 19:45:59 joerg Exp $

#
# This Makefile exists to provide a single point to build
# all libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKLDAP} != "no")
SUBDIR+= ../bsd/openldap/lib
.endif

SUBDIR+= ../bsd/libarchive/lib

.include <bsd.subdir.mk>
