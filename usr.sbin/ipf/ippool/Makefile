#	$NetBSD: Makefile,v 1.3.2.3 2004/03/31 07:59:38 tron Exp $

PROG=		ippool
SRCS=		ippool_y.c ippool_l.c kmem.c ippool.c
MAN=		ippool.8
DPADD+=		${LIBKVM}
LDADD+=		-lkvm
CPPFLAGS+=	-I.

CLEANFILES+=	ippool_y.c ippool_y.h
CLEANFILES+=	ippool_l.c ippool_l.h

ippool_y.c: ippool_y.y ippool_l.h
	${_MKTARGET_CREATE}
	${YACC} -d ${NETBSDSRCDIR}/dist/ipf/tools/ippool_y.y
	sed -e 's/yy/ippool_yy/g' \
	    -e 's/"ippool_y.y"/"..\/tools\/ippool_y.y"/' \
		y.tab.c > ${.TARGET}
	sed -e 's/yy/ippool_yy/g' \
		y.tab.h > ${.TARGET:.c=.h}

ippool_y.h: ippool_y.c

ippool_l.c: lexer.c ippool_l.h ippool_y.h
	${_MKTARGET_CREATE}
	sed -e 's/yy/ippool_yy/g' \
	     -e 's/y.tab.h/ippool_y.h/' \
	     -e 's/lexer.h/ippool_l.h/' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.c > ${.TARGET}

ippool_l.h: lexer.h
	${_MKTARGET_CREATE}
	sed -e 's/yy/ippool_yy/g' \
	    ${.ALLSRC} > ${.TARGET}

.include <bsd.prog.mk>
