#	$NetBSD: Makefile,v 1.1.4.1 2008/10/19 22:39:27 haad Exp $

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
