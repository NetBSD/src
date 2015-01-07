#	$NetBSD: Makefile,v 1.7.4.1 2015/01/07 04:45:13 msaitoh Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils dhcpcd fetch libarchive ntp pkg_install
.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif
.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif
SUBDIR+= top

.include <bsd.subdir.mk>
