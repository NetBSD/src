#	$NetBSD: Makefile,v 1.1.36.1 2020/04/08 14:04:05 martin Exp $

PROG=		ipmon
SRCS=		ipmon.c ipmon_y.c ipmon_l.c
MAN=		ipmon.5 ipmon.8

CPPFLAGS+=	-DLOGFAC=LOG_LOCAL0 -I.

DPSRCS+=	ipmon_l.h ipmon_y.h

CLEANFILES+=	ipmon_y.c ipmon_y.h
CLEANFILES+=	ipmon_l.c ipmon_l.h

COPTS.ipmon.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} == 8:? -Wno-error=format-overflow :}

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

.include <bsd.prog.mk>
