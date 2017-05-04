#	$NetBSD: Makefile,v 1.2.8.1 2017/05/04 06:03:46 snj Exp $

SUBDIR= ntp-keygen ntpd ntpdate ntpdc ntpq ntptime ntptrace sntp 

.if ("${MKCRYPTO}" != "no")
CPPFLAGS+=-DOPENSSL
.endif

.include "Makefile.inc"

.include <bsd.subdir.mk>
