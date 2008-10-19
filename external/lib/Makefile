#	$NetBSD: Makefile,v 1.1.4.1 2008/10/19 22:40:55 haad Exp $

#
# This Makefile exists to provide a single point to build
# all libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKLDAP} != "no")
SUBDIR+= ../bsd/openldap/lib
.endif

SUBDIR+= ../bsd/fetch/lib
SUBDIR+= ../bsd/libarchive/lib

.include <bsd.subdir.mk>
