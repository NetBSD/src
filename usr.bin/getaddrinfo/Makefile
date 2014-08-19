#	$NetBSD: Makefile,v 1.2.6.2 2014/08/20 00:04:59 tls Exp $

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
