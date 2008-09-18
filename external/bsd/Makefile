#	$NetBSD: Makefile,v 1.4.2.2 2008/09/18 05:14:26 wrstuden Exp $

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
