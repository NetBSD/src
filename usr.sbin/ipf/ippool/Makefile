#	$NetBSD: Makefile,v 1.10 2009/11/13 09:51:13 mishka Exp $

PROG=		ippool
SRCS=		ippool_y.c ippool_l.c kmem.c ippool.c
MAN=		ippool.5 ippool.8
MLINKS=		ippool.5 ippool.conf.5
CPPFLAGS+=	-I.

DPSRCS+=	ippool_l.h ippool_y.h

CLEANFILES+=	ippool_y.c ippool_y.h
CLEANFILES+=	ippool_l.c ippool_l.h

ippool_y.c: ippool_y.y
	${_MKTARGET_CREATE}
	${YACC} -d ${.ALLSRC}
	${TOOL_SED} -e 's/yy/ippool_yy/g' \
	    -e 's/"ippool_y.y"/"..\/tools\/ippool_y.y"/' \
	    y.tab.c > ${.TARGET}
	${TOOL_SED} -e 's/yy/ippool_yy/g' \
	    y.tab.h > ${.TARGET:.c=.h}

ippool_y.h: ippool_y.c

ippool_l.c: lexer.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's/yy/ippool_yy/g' \
	    -e 's/y.tab.h/ippool_y.h/' \
	    -e 's/lexer.h/ippool_l.h/' \
	    ${.ALLSRC} > ${.TARGET}

ippool_l.h: lexer.h
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's/yy/ippool_yy/g' \
	    ${.ALLSRC} > ${.TARGET}

.include <bsd.prog.mk>
