#	$NetBSD: Makefile,v 1.7.20.1 2007/09/03 07:05:38 wrstuden Exp $

.include <bsd.own.mk>

SUBDIR=	libopts libisc libntp libparse .WAIT
SUBDIR+= ntpd ntpdate ntpdc ntpq ntptime
SUBDIR+= ntptrace html

.if (${MKCRYPTO} != "no")
SUBDIR+= ntp-keygen
.endif

.include <bsd.subdir.mk>
