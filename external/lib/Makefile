#	$NetBSD: Makefile,v 1.16 2010/06/09 23:02:44 christos Exp $

#
# This Makefile exists to provide a single point to build
# all libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKATF} != "no")
SUBDIR+= ../bsd/atf/lib
.endif

.if (${MKMDNS} != "no")
SUBDIR+= ../apache2/mDNSResponder/lib
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+= ../../crypto/external/lib .WAIT
.endif

.if (${MKLDAP} != "no")
SUBDIR+= ../bsd/openldap/lib
.endif

.if (${MKISCSI} != "no")
SUBDIR+=	../bsd/iscsi/lib	# depends on libpthread
.endif

SUBDIR+= ../bsd/fetch/lib
SUBDIR+= ../bsd/libarchive/lib
SUBDIR+= ../bsd/libevent/lib
SUBDIR+= ../bsd/libelf/lib
SUBDIR+= ../bsd/libdwarf/lib
SUBDIR+= ../bsd/am-utils/lib
SUBDIR+= ../bsd/bind/lib
SUBDIR+= ../bsd/file/lib

.include <bsd.subdir.mk>
