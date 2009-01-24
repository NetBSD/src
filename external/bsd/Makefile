#	$NetBSD: Makefile,v 1.9 2009/01/24 01:15:26 agc Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils dhcpcd fetch libarchive pkg_install
.if (${MKATF} != "no")
SUBDIR+= atf
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
SUBDIR+= top

.include <bsd.subdir.mk>
