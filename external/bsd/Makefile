#	$NetBSD: Makefile,v 1.9.2.1 2009/05/13 18:49:01 jym Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind dhcpcd fetch file libarchive pkg_install top
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

.include <bsd.subdir.mk>
