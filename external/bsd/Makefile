#	$NetBSD: Makefile,v 1.5 2008/09/19 21:41:51 christos Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils dhcpcd
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif
SUBDIR+= top

.include <bsd.subdir.mk>
