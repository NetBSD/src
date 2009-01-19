#	$NetBSD: Makefile,v 1.8 2009/01/19 07:14:06 jmmv Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils dhcpcd fetch libarchive pkg_install
.if (${MKATF} != "no")
SUBDIR+= atf
.endif
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif
SUBDIR+= top

.include <bsd.subdir.mk>
