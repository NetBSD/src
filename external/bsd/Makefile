#	$NetBSD: Makefile,v 1.19 2009/11/02 10:13:19 plunky Exp $

.include <bsd.own.mk>

SUBDIR+= am-utils bind dhcpcd fetch file flex libarchive libevent \
	mdocml pkg_install top byacc

.if (${MKATF} != "no")
SUBDIR+= atf
.endif
.if (${MKISCSI} != "no")
SUBDIR+= iscsi 
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
