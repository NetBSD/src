#	$NetBSD: Makefile,v 1.3 2008/07/27 19:31:03 joerg Exp $

.include <bsd.own.mk>

SUBDIR+= dhcpcd
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
SUBDIR+= top

.include <bsd.subdir.mk>
