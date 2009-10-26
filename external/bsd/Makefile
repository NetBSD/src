#	$NetBSD: Makefile,v 1.15 2009/10/26 04:10:12 christos Exp $

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
