#	$NetBSD: Makefile,v 1.2.28.2 2015/01/07 04:45:13 msaitoh Exp $

SUBDIR= ntp-keygen ntpd ntpdate ntpdc ntpq ntptime ntptrace sntp 

.if ("${MKCRYPTO}" != "no")
SUBDIR+= ntp-keygen
CPPFLAGS+=-DOPENSSL
.endif

.include "Makefile.inc"

.include <bsd.subdir.mk>
