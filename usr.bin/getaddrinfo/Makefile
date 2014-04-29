#	$NetBSD: Makefile,v 1.2 2014/04/29 01:21:02 christos Exp $

.include <bsd.own.mk>

PROG=	getaddrinfo

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

WARNS=	5

SYS_SOCKET_H?=	${NETBSDSRCDIR}/sys/sys/socket.h

CPPFLAGS+=	-I.
DPSRCS+=	tables.h
CLEANFILES+=	tables.h
tables.h: tables.awk ${SYS_SOCKET_H}
	${_MKTARGET_CREATE}
	${TOOL_AWK} -f ${.ALLSRC} > ${.TARGET}.tmp \
	&& mv -f -- ${.TARGET}.tmp ${.TARGET}

.include <bsd.prog.mk>
