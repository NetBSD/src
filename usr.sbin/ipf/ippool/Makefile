#	$NetBSD: Makefile,v 1.2 2004/03/28 09:35:26 mrg Exp $

PROG=		ippool
SRCS=		ippool_y.c ippool_l.c kmem.c ippool.c
MAN=		ippool.8
DPADD+=		${LIBL} ${LIBKVM}
LDADD+=		-ll -lkvm -lipf

CLEANFILES+=	ippool_y.c ippool_y.h
CLEANFILES+=	ippool_l.c ippool_l.h

ippool_y.c ippool_y.h: ippool_y.y ippool_l.h
	yacc -d ${NETBSDSRCDIR}/dist/ipf/tools/ippool_y.y
	sed -e 's/yy/ippool_yy/g' \
	    -e 's/"ippool_y.y"/"..\/tools\/ippool_y.y"/' \
		y.tab.c > ippool_y.c
	sed -e 's/yy/ippool_yy/g' \
		y.tab.h > ippool_y.h

ippool_l.c: lexer.c ippool_l.h ippool_y.h
	sed -e 's/yy/ippool_yy/g' \
	     -e 's/y.tab.h/ippool_y.h/' \
	     -e 's/lexer.h/ippool_l.h/' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.c > ippool_l.c

ippool_l.h: lexer.c
	sed -e 's/yy/ippool_yy/g' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.h > ippool_l.h

.include <bsd.prog.mk>
