#	$NetBSD: Makefile,v 1.13 2009/10/21 22:59:06 joerg Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind dhcpcd fetch file libarchive mdocml pkg_install top
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
