#	$NetBSD: Makefile,v 1.17 2004/03/28 09:01:28 martti Exp $

PROG=		ipnat
SRCS=		ipnat.c ipnat_y.c ipnat_l.c
MAN=		ipnat.8 ipnat.4 ipnat.5
MLINKS=		ipnat.5 ipnat.conf.5
DPADD+=		${LIBKVM} ${LIBL}
LDADD+=		-lkvm -ll -lipf

CLEANFILES+=	ipnat_y.c ipnat_y.h
CLEANFILES+=	ipnat_l.c ipnat_l.h

ipnat_y.c ipnat_y.h: ipnat_y.y ipnat_l.h
	yacc -d ${NETBSDSRCDIR}/dist/ipf/tools/ipnat_y.y
	sed -e 's/yy/ipnat_yy/g' \
	    -e 's/y.tab.c/ipnat_y.c/' \
	    -e s/\"ipnat_y.y\"/\"..\\/tools\\/ipnat_y.y\"/ \
		y.tab.c > ipnat_y.c
	sed -e 's/yy/ipnat_yy/g' \
	    -e 's/y.tab.h/ipnat_y.h/' \
		y.tab.h > ipnat_y.h

ipnat_l.c: lexer.c ipnat_l.h
	sed -e 's/yy/ipnat_yy/g' \
	    -e 's/y.tab.h/ipnat_y.h/' \
	    -e 's/lexer.h/ipnat_l.h/' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.c > ipnat_l.c

ipnat_l.h: lexer.c
	sed -e 's/yy/ipnat_yy/g' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.h > ipnat_l.h

.include <bsd.prog.mk>
