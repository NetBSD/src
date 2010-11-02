#	$NetBSD: Makefile,v 1.17 2010/11/02 16:34:35 joerg Exp $

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

SUBDIR+= ../public-domain/xz/lib .WAIT

SUBDIR+= ../bsd/fetch/lib
SUBDIR+= ../bsd/libarchive/lib
SUBDIR+= ../bsd/libevent/lib
SUBDIR+= ../bsd/libelf/lib
SUBDIR+= ../bsd/libdwarf/lib
SUBDIR+= ../bsd/am-utils/lib
SUBDIR+= ../bsd/bind/lib
SUBDIR+= ../bsd/file/lib

.include <bsd.subdir.mk>
