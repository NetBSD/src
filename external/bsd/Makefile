#	$NetBSD: Makefile,v 1.25 2010/08/28 15:47:08 kardel Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind cron dhcpcd fetch file flex libarchive libevent liblzf \
	mdocml openresolv top byacc wpa ntp

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
