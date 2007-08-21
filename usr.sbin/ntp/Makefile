#	$NetBSD: Makefile,v 1.7.18.1 2007/08/21 08:40:53 ghen Exp $

.include <bsd.own.mk>

SUBDIR=	libopts libisc libntp libparse .WAIT
SUBDIR+= ntpd ntpdate ntpdc ntpq ntptime
SUBDIR+= ntptrace html

.if (${MKCRYPTO} != "no")
SUBDIR+= ntp-keygen
.endif

.include <bsd.subdir.mk>
