#	$NetBSD: Makefile,v 1.11 2009/06/25 13:59:02 agc Exp $

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

.if (${MKISCSI} != "no")
SUBDIR+=	../bsd/iscsi/lib	# depends on libpthread
.endif

SUBDIR+= ../bsd/fetch/lib
SUBDIR+= ../bsd/libarchive/lib
SUBDIR+= ../bsd/am-utils/lib
SUBDIR+= ../bsd/bind/lib
SUBDIR+= ../bsd/file/lib

.include <bsd.subdir.mk>
