#	$NetBSD: Makefile,v 1.1 2000/03/29 13:13:42 simonb Exp $

SUBDIR=	libntp ntpd ntpdate ntpdc ntpq ntptimeset ntptrace

all-ntpd all-ntpdate all-ntpdc all-ntpq all-ntptimeset all-ntptrace: all-libntp
depend-ntpd depend-ntpdate depend-ntpdc depend-ntpq depend-ntptimeset \
	depend-ntptrace: depend-libntp

.include <bsd.subdir.mk>
