#	$NetBSD: Makefile,v 1.9 2007/01/20 16:50:29 kardel Exp $

.include <bsd.own.mk>

SUBDIR=	libopts libisc libntp libparse .WAIT
SUBDIR+= ntpd ntpdate ntpdc ntpq ntptime
SUBDIR+= ntptrace html

.if (${MKCRYPTO} != "no")
SUBDIR+= ntp-keygen
.endif

.include <bsd.subdir.mk>
