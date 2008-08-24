#	$NetBSD: Makefile,v 1.4 2008/08/24 05:47:05 gmcgarry Exp $

.include <bsd.own.mk>

SUBDIR+= dhcpcd
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif
SUBDIR+= top

.include <bsd.subdir.mk>
