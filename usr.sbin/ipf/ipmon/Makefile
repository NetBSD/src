#	$NetBSD: Makefile,v 1.20 2004/03/31 00:34:32 lukem Exp $

PROG=		ipmon
SRCS=		ipmon.c ipmon_y.c ipmon_l.c
MAN=		ipmon.8

CPPFLAGS+=	-DLOGFAC=LOG_LOCAL0 -I.

DPSRCS+=	ipmon_l.h ipmon_y.h

CLEANFILES+=	ipmon_y.c ipmon_y.h
CLEANFILES+=	ipmon_l.c ipmon_l.h

ipmon_y.c: ipmon_y.y
	${_MKTARGET_CREATE}
	${YACC} -d ${.ALLSRC}
	sed -e 's/yy/ipmon_yy/g' \
	    -e 's/"ipmon_y.y"/"..\/tools\/ipmon_y.y"/' \
	    y.tab.c > ${.TARGET}
	sed -e 's/yy/ipmon_yy/g' \
	    y.tab.h > ${.TARGET:.c=.h}

ipmon_y.h: ipmon_y.c

ipmon_l.c: lexer.c
	${_MKTARGET_CREATE}
	sed -e 's/yy/ipmon_yy/g' \
	    -e 's/y.tab.h/ipmon_y.h/' \
	    -e 's/lexer.h/ipmon_l.h/' \
	    ${.ALLSRC} > ${.TARGET}

ipmon_l.h: lexer.h
	${_MKTARGET_CREATE}
	sed -e 's/yy/ipmon_yy/g' \
	    ${.ALLSRC} > ${.TARGET}

.include <bsd.prog.mk>
