#	$NetBSD: Makefile,v 1.26 2004/03/29 23:28:12 jmc Exp $

NOGCCERROR=	# defined

.include <bsd.own.mk>

PROG=		ipftest
SRCS=		ipftest.c fil.c ip_frag.c ip_state.c ip_nat.c \
		ip_proxy.c ip_auth.c ip_htable.c ip_lookup.c \
		ip_pool.c ip_scan.c ip_sync.c ip_rules.c \
		ip_fil.c ip_log.c ippool_y.c ippool_l.c ipf_y.c \
		ipf_l.c ipnat_y.c ipnat_l.c md5.c radix.c bpf_filter.c
MAN=		ipftest.1

CPPFLAGS+=	-DIPFILTER_LOG -DIPFILTER_COMPILED -DIPFILTER_LOOKUP \
		-DIPFILTER_SCAN -DIPFILTER_SYNC -DIPFILTER_CKSUM -I.

.PATH:		${NETBSDSRCDIR}/sys/netinet

CLEANFILES+=	ipf_y.c ipf_y.h
CLEANFILES+=	ipf_l.c ipf_l.h
CLEANFILES+=	ipf.tab.c ipf.tab.h
CLEANFILES+=	ipnat_y.c ipnat_y.h
CLEANFILES+=	ipnat_l.c ipnat_l.h
CLEANFILES+=	ipnat.tab.c ipnat.tab.h
CLEANFILES+=	ippool_y.c ippool_y.h
CLEANFILES+=	ippool_l.c ippool_l.h
CLEANFILES+=	ippool.tab.c ippool.tab.h

ipnat_y.c: ipnat_y.y ipnat_l.h
	${_MKTARGET_CREATE}
	${YACC} -b ipnat -d ${NETBSDSRCDIR}/dist/ipf/tools/ipnat_y.y
	sed -e 's/yy/ipnat_yy/g' \
	    -e 's/y.tab.c/ipnat_y.c/' \
	    -e s/\"ipnat_y.y\"/\"..\\/tools\\/ipnat_y.y\"/ \
		ipnat.tab.c > ${.TARGET}
	sed -e 's/yy/ipnat_yy/g' \
	    -e 's/y.tab.h/ipnat_y.h/' \
		ipnat.tab.h > ${.TARGET:.c=.h}

ipnat_y.h: ipnat_y.c

ipnat_l.c: lexer.c ipnat_l.h ipnat_y.h
	${_MKTARGET_CREATE}
	sed -e 's/yy/ipnat_yy/g' \
	    -e 's/y.tab.h/ipnat_y.h/' \
	    -e 's/lexer.h/ipnat_l.h/' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.c > ${.TARGET}

ipnat_l.h: lexer.h
	${_MKTARGET_CREATE}
	sed -e 's/yy/ipnat_yy/g' \
	    ${.ALLSRC} > ${.TARGET}

ippool_y.c: ippool_y.y ippool_l.h
	${_MKTARGET_CREATE}
	${YACC} -b ippool -d ${NETBSDSRCDIR}/dist/ipf/tools/ippool_y.y
	sed -e 's/yy/ippool_yy/g' \
	    -e 's/"ippool_y.y"/"..\/tools\/ippool_y.y"/' \
		ippool.tab.c > ${.TARGET}
	sed -e 's/yy/ippool_yy/g' \
		ippool.tab.h > ${.TARGET:.c=.h}

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

ipf_y.c: ipf_y.y ipf_l.h
	${_MKTARGET_CREATE}
	${YACC} -b ipf -d ${NETBSDSRCDIR}/dist/ipf/tools/ipf_y.y
	sed -e 's/yy/ipf_yy/g' \
	    -e 's/"ipf_y.y"/"..\/tools\/ipf_y.y"/' \
		ipf.tab.c > ${.TARGET}
	sed -e 's/yy/ipf_yy/g' \
		ipf.tab.h > ${.TARGET:.c=.h}

ipf_y.h: ipf_y.c

ipf_l.c: lexer.c ipf_l.h ipf_y.h
	${_MKTARGET_CREATE}
	sed -e 's/yy/ipf_yy/g' \
	     -e 's/y.tab.h/ipf_y.h/' \
	     -e 's/lexer.h/ipf_l.h/' \
		${NETBSDSRCDIR}/dist/ipf/tools/lexer.c > ${.TARGET}

ipf_l.h: lexer.h
	${_MKTARGET_CREATE}
	sed -e 's/yy/ipf_yy/g' \
	    ${.ALLSRC} > ${.TARGET}

.include <bsd.prog.mk>
