#	$NetBSD: Makefile,v 1.24 2010/08/04 17:12:33 christos Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind cron dhcpcd fetch file flex libarchive libevent liblzf \
	mdocml openresolv top byacc wpa

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
