#	$NetBSD: Makefile,v 1.16 2004/03/28 09:35:26 mrg Exp $

PROG=		ipmon
SRCS=		ipmon.c ipmon_y.c ipmon_l.c
MAN=		ipmon.8
DPADD+=		${LIBL}
LDADD+=		-ll -lipf

CPPFLAGS+=	-DLOGFAC=LOG_LOCAL0

CLEANFILES+=	ipmon_y.c ipmon_y.h
CLEANFILES+=	ipmon_l.c ipmon_l.h

ipmon_y.c ipmon_y.h: ipmon_y.y ipmon_l.h
	yacc -d ${NETBSDSRCDIR}/dist/ipf/tools/ipmon_y.y
	sed -e 's/yy/ipmon_yy/g' \
	    -e 's/"ipmon_y.y"/"..\/tools\/ipmon_y.y"/' \
		y.tab.c > ipmon_y.c
	sed -e 's/yy/ipmon_yy/g' \
		y.tab.h > ipmon_y.h

ipmon_l.c: lexer.c ipmon_l.h ipmon_y.h
	sed -e 's/yy/ipmon_yy/g' \
	     -e 's/y.tab.h/ipmon_y.h/' \
	     -e 's/lexer.h/ipmon_l.h/' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.c > ipmon_l.c

ipmon_l.h: lexer.c
	sed -e 's/yy/ipmon_yy/g' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.h > ipmon_l.h

.include <bsd.prog.mk>
