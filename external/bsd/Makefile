#	$NetBSD: Makefile,v 1.23 2010/07/11 06:16:54 mrg Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind cron dhcpcd fetch file flex libarchive libevent liblzf \
	mdocml openresolv top byacc

.if (${MKATF} != "no")
SUBDIR+= atf
.endif
.if (${MKISCSI} != "no")
SUBDIR+= iscsi 
.endif
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
.if (${MKCRYPTO} != "no")
SUBDIR+= pkg_install ../../crypto/external/bsd
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif

.include <bsd.subdir.mk>
