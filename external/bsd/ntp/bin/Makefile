#	$NetBSD: Makefile,v 1.2.30.2 2015/01/07 10:09:54 msaitoh Exp $

SUBDIR= ntp-keygen ntpd ntpdate ntpdc ntpq ntptime ntptrace sntp 

.if ("${MKCRYPTO}" != "no")
SUBDIR+= ntp-keygen
CPPFLAGS+=-DOPENSSL
.endif

.include "Makefile.inc"

.include <bsd.subdir.mk>
