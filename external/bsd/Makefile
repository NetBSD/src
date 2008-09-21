#	$NetBSD: Makefile,v 1.6 2008/09/21 19:45:59 joerg Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils dhcpcd libarchive
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif
SUBDIR+= top

.include <bsd.subdir.mk>
