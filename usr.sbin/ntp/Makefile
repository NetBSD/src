#	$NetBSD: Makefile,v 1.2 2000/03/29 14:35:45 simonb Exp $

SUBDIR=	libntp ntpd ntpdate ntpdc ntpq ntptime ntptimeset ntptrace

all-ntpd all-ntpdate all-ntpdc all-ntpq all-ntptime all-ntptimeset \
	all-ntptrace: all-libntp
depend-ntpd depend-ntpdate depend-ntpdc depend-ntpq depend-ntptime \
	depend-ntptimeset depend-ntptrace: depend-libntp

.include <bsd.subdir.mk>
