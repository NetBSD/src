#	$NetBSD: Makefile,v 1.5 2009/01/19 07:16:11 jmmv Exp $

#
# This Makefile exists to provide a single point to build
# all libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKATF} != "no")
SUBDIR+= ../bsd/atf/lib
.endif

.if (${MKLDAP} != "no")
SUBDIR+= ../bsd/openldap/lib
.endif

SUBDIR+= ../bsd/fetch/lib
SUBDIR+= ../bsd/libarchive/lib
SUBDIR+= ../bsd/am-utils/lib

.include <bsd.subdir.mk>
