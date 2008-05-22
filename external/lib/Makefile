#	$NetBSD: Makefile,v 1.1 2008/05/22 14:12:21 lukem Exp $

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
