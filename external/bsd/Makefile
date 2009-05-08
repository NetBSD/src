#	$NetBSD: Makefile,v 1.11 2009/05/08 17:45:49 christos Exp $

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
