#	$NetBSD: Makefile,v 1.2.22.1 2017/05/04 06:00:50 snj Exp $

SUBDIR= ntp-keygen ntpd ntpdate ntpdc ntpq ntptime ntptrace sntp 

.if ("${MKCRYPTO}" != "no")
CPPFLAGS+=-DOPENSSL
.endif

.include "Makefile.inc"

.include <bsd.subdir.mk>
