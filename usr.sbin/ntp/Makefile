#	$NetBSD: Makefile,v 1.8 2007/01/06 19:45:29 kardel Exp $

.include <bsd.own.mk>

SUBDIR=	libopts libisc libntp libparse .WAIT
SUBDIR+= ntpd ntpdate ntpdc ntpq ntptime
SUBDIR+= ntptrace

.if (${MKCRYPTO} != "no")
SUBDIR+= ntp-keygen
.endif

.include <bsd.subdir.mk>
