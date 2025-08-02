#	$NetBSD: Makefile.inc,v 1.1.4.2 2025/08/02 05:58:27 perseant Exp $

SYS_SOCKET_H?=	${NETBSDSRCDIR}/sys/sys/socket.h

CPPFLAGS+=	-I.
DPSRCS+=	tables.h
CLEANFILES+=	tables.h
tables.h: ${.PARSEDIR}/tables.awk ${SYS_SOCKET_H}
	${_MKTARGET_CREATE}
	${TOOL_AWK} -f ${.ALLSRC} > ${.TARGET}.tmp \
	&& mv -f -- ${.TARGET}.tmp ${.TARGET}
