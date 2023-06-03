#	$NetBSD: Makefile,v 1.7 2023/06/03 21:26:52 lukem Exp $

PROG=		ipmon
SRCS=		ipmon.c ipmon_y.c ipmon_l.c
MAN=		ipmon.5 ipmon.8

CPPFLAGS+=	-DLOGFAC=LOG_LOCAL0 -I.

DPSRCS+=	ipmon_l.h ipmon_y.h

CLEANFILES+=	ipmon_y.c ipmon_y.h
CLEANFILES+=	ipmon_l.c ipmon_l.h

ipmon_y.c: ipmon_y.y
	${_MKTARGET_CREATE}
	${YACC} -d ${.ALLSRC}
	${TOOL_SED} -e 's/yy/ipmon_yy/g' \
	    -e 's/"ipmon_y.y"/"..\/tools\/ipmon_y.y"/' \
	    y.tab.c > ${.TARGET}
	${TOOL_SED} -e 's/yy/ipmon_yy/g' \
	    y.tab.h > ${.TARGET:.c=.h}

ipmon_y.h: ipmon_y.c

ipmon_l.c: lexer.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's/yy/ipmon_yy/g' \
	    -e 's/y.tab.h/ipmon_y.h/' \
	    -e 's/lexer.h/ipmon_l.h/' \
	    ${.ALLSRC} > ${.TARGET}

ipmon_l.h: lexer.h
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's/yy/ipmon_yy/g' \
	    ${.ALLSRC} > ${.TARGET}

COPTS.ipmon.c+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER} ${CC_WNO_FORMAT_OVERFLOW}

.include <bsd.prog.mk>
