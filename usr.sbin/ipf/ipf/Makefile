#	$NetBSD: Makefile,v 1.18 2004/03/29 04:40:00 lukem Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=		ipf
SRCS=		ipf.c ipfcomp.c ipf_y.c ipf_l.c
MAN=		ipf.8 ipf.4 ipf.5 ipl.4
MLINKS=		ipl.4 ipfilter.4 ipf.5 ipf.conf.5 ipf.5 ipf6.conf.5

CLEANFILES+=	ipf_y.c ipf_y.h
CLEANFILES+=	ipf_l.c ipf_l.h

ipf_y.c: ipf_y.y ipf_l.h
	${_MKTARGET_CREATE}
	${YACC} -d ${NETBSDSRCDIR}/dist/ipf/tools/ipf_y.y
	sed -e 's/yy/ipf_yy/g' \
	    -e 's/"ipf_y.y"/"..\/tools\/ipf_y.y"/' \
		y.tab.c > ${.TARGET}
	sed -e 's/yy/ipf_yy/g' \
		y.tab.h > ${.TARGET:.c=.h}

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

BINDIR=		/sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.include <bsd.prog.mk>
