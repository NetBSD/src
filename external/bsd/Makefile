#	$NetBSD: Makefile,v 1.10 2009/04/12 17:13:42 christos Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind dhcpcd fetch libarchive pkg_install top
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
