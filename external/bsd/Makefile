#	$NetBSD: Makefile,v 1.22 2010/05/07 17:35:32 christos Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind cron dhcpcd fetch file flex libarchive libevent liblzf \
	mdocml openresolv pkg_install top byacc

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
SUBDIR+= ../../crypto/external/bsd
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif

.include <bsd.subdir.mk>
