#	$NetBSD: Makefile,v 1.1 2009/12/13 17:13:48 kardel Exp $

SUBDIR= ntp-keygen ntpd ntpdate ntpdc ntpq ntptime

.if ("${MKCRYPTO}" != "no")
SUBDIR+= ntp-keygen
CPPFLAGS+=-DOPENSSL
.endif

.include "Makefile.inc"

.include <bsd.subdir.mk>
