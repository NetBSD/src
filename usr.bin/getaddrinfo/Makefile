#	$NetBSD: Makefile,v 1.1 2013/09/30 06:19:22 riastradh Exp $

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
