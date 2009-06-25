#	$NetBSD: Makefile,v 1.12 2009/06/25 13:59:02 agc Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind dhcpcd fetch file libarchive pkg_install top
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
