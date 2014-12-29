#	$NetBSD: Makefile,v 1.11.40.1 2014/12/29 17:27:28 martin Exp $

.include <bsd.own.mk>

LIBISPRIVATE=	yes

LIB=	common
SRCS=	common.c common_vars.c displayq.c fatal.c rmjob.c startdaemon.c

.if (${USE_INET6} != "no")
CPPFLAGS.common.c=	-DINET6
.endif

.include <bsd.lib.mk>
