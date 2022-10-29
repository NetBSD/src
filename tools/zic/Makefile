#	$NetBSD: Makefile,v 1.7 2022/10/29 13:53:35 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}zic
HOST_SRCDIR=	usr.sbin/zic

.if !empty(HOST_OSTYPE:MSunOS-5.1[01]-*)
HOST_CPPFLAGS+=	-D_POSIX_PTHREAD_SEMANTICS
.endif
HOST_CPPFLAGS+= -DHAVE_GETTEXT=0

.include "${.CURDIR}/../Makefile.host"
