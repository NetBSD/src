#	$NetBSD: Makefile,v 1.22 2004/03/28 09:01:27 martti Exp $

.include <bsd.own.mk>

PROG=		ipftest
SRCS=		ipftest.c fil.c ip_frag.c ip_state.c ip_nat.c \
		ip_proxy.c ip_auth.c ip_htable.c ip_lookup.c \
		ip_pool.c ip_scan.c ip_sync.c ip_rules.c \
		ip_fil.c ip_log.c ippool_y.c ippool_l.c ipf_y.c \
		ipf_l.c ipnat_y.c ipnat_l.c md5.c radix.c bpf_filter.c
MAN=		ipftest.1
DPADD+=		${LIBL}
LDADD+=		-ll -lipf

CPPFLAGS+=	-DIPFILTER_LOG -DIPFILTER_COMPILED -DIPFILTER_LOOKUP \
		-DIPFILTER_SCAN -DIPFILTER_SYNC -DIPFILTER_CKSUM

.PATH:		${NETBSDSRCDIR}/sys/netinet

NOGCCERROR=	# defined

CLEANFILES+=	ipf_y.c ipf_y.h
CLEANFILES+=	ipf_l.c ipf_l.h
CLEANFILES+=	ipnat_y.c ipnat_y.h
CLEANFILES+=	ipnat_l.c ipnat_l.h
CLEANFILES+=	ippool_y.c ippool_y.h
CLEANFILES+=	ippool_l.c ippool_l.h

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

ippool_y.c ippool_y.h: ippool_y.y ippool_l.h
	yacc -d ${NETBSDSRCDIR}/dist/ipf/tools/ippool_y.y
	sed -e 's/yy/ippool_yy/g' \
	    -e 's/"ippool_y.y"/"..\/tools\/ippool_y.y"/' \
		y.tab.c > ippool_y.c
	sed -e 's/yy/ippool_yy/g' \
		y.tab.h > ippool_y.h

ippool_l.c: lexer.c ippool_l.h
	sed -e 's/yy/ippool_yy/g' \
	    -e 's/y.tab.h/ippool_y.h/' \
	    -e 's/lexer.h/ippool_l.h/' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.c > ippool_l.c

ippool_l.h: lexer.h
	sed -e 's/yy/ippool_yy/g' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.h > ippool_l.h

ipf_y.c ipf_y.h: ipf_y.y ipf_l.h
	yacc -d ${NETBSDSRCDIR}/dist/ipf/tools/ipf_y.y
	sed -e 's/yy/ipf_yy/g' \
	    -e 's/"ipf_y.y"/"..\/tools\/ipf_y.y"/' \
		y.tab.c > ipf_y.c
	sed -e 's/yy/ipf_yy/g' \
		y.tab.h > ipf_y.h

ipf_l.c: lexer.c ipf_l.h ipf_y.h
	sed -e 's/yy/ipf_yy/g' \
	     -e 's/y.tab.h/ipf_y.h/' \
	     -e 's/lexer.h/ipf_l.h/' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.c > ipf_l.c

ipf_l.h: lexer.c
	sed -e 's/yy/ipf_yy/g' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.h > ipf_l.h

.include <bsd.prog.mk>
