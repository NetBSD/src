#	$NetBSD: Makefile,v 1.5 2010/03/07 19:10:10 hans Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}zic
HOST_SRCDIR=	usr.sbin/zic

.if !empty(HOST_OSTYPE:MSunOS-5.11-*)
HOST_CPPFLAGS=	-D_POSIX_PTHREAD_SEMANTICS
.endif

.include "${.CURDIR}/../Makefile.host"
