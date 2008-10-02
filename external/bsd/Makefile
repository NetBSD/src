#	$NetBSD: Makefile,v 1.7 2008/10/02 17:54:52 joerg Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils dhcpcd fetch libarchive pkg_install
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif
SUBDIR+= top

.include <bsd.subdir.mk>
