#	$NetBSD: Makefile,v 1.7 2004/01/04 00:06:34 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	libisc libntp libparse .WAIT
SUBDIR+= ntpd ntpdate ntpdc ntpq ntptime
SUBDIR+= ntptrace

.if (${MKCRYPTO} != "no")
SUBDIR+= ntp-keygen
.endif

.include <bsd.subdir.mk>
