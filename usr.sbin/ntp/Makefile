#	$NetBSD: Makefile,v 1.3 2000/05/02 12:01:10 simonb Exp $

SUBDIR=	libntp ntpd ntpdate ntpdc ntpq ntptime ntptimeset ntptrace html

all-ntpd all-ntpdate all-ntpdc all-ntpq all-ntptime all-ntptimeset \
	all-ntptrace: all-libntp
depend-ntpd depend-ntpdate depend-ntpdc depend-ntpq depend-ntptime \
	depend-ntptimeset depend-ntptrace: depend-libntp

.include <bsd.subdir.mk>
