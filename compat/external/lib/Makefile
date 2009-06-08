#	$NetBSD: Makefile,v 1.6 2009/06/08 01:22:53 christos Exp $

#
# This Makefile exists to provide a single point to build
# all compat libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKATF} != "no")
SUBDIR+= ../bsd/atf/lib
.endif

.if (${MKCRYPTO} != "no")
.if (${MKLDAP} != "no")
SUBDIR+= ../bsd/openldap/lib
.endif
.endif

SUBDIR+= ../bsd/bind/lib
SUBDIR+= ../bsd/file/lib

.include <bsd.subdir.mk>
