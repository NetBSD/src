#	$NetBSD: Makefile,v 1.1.6.2 2008/09/18 05:17:57 wrstuden Exp $

#
# This Makefile exists to provide a single point to build
# all libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKLDAP} != "no")
SUBDIR+= ../bsd/openldap/lib
.endif

.include <bsd.subdir.mk>
