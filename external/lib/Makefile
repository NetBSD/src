#	$NetBSD: Makefile,v 1.1.2.2 2008/06/04 02:04:32 yamt Exp $

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
