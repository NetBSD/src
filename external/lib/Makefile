#	$NetBSD: Makefile,v 1.6.2.1 2009/05/13 18:53:23 jym Exp $

#
# This Makefile exists to provide a single point to build
# all libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKATF} != "no")
SUBDIR+= ../bsd/atf/lib
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+= ../../crypto/external/lib .WAIT

.if (${MKLDAP} != "no")
SUBDIR+= ../bsd/openldap/lib
.endif
.endif

SUBDIR+= ../bsd/fetch/lib
SUBDIR+= ../bsd/libarchive/lib
SUBDIR+= ../bsd/am-utils/lib
SUBDIR+= ../bsd/bind/lib
SUBDIR+= ../bsd/file/lib

.include <bsd.subdir.mk>
