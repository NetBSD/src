#	$NetBSD: Makefile.inc,v 1.11 2004/03/28 09:01:27 martti Exp $

.include <bsd.own.mk>

CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf
CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf/tools
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/netinet
CPPFLAGS+=	-DSTATETOP -D__UIO_EXPOSE

IPFOBJDIR!=	cd $(.CURDIR)/../libipf && ${PRINTOBJDIR}
LDFLAGS+=	-L${IPFOBJDIR}

CLEANFILES+=	y.tab.c y.tab.h

.PATH:	${NETBSDSRCDIR}/dist/ipf	\
	${NETBSDSRCDIR}/dist/ipf/lib	\
	${NETBSDSRCDIR}/dist/ipf/tools	\
	${NETBSDSRCDIR}/dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
