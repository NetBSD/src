#	$NetBSD: Makefile.inc,v 1.14 2004/10/01 15:26:35 christos Exp $

.include <bsd.own.mk>

CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf
CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf/tools
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/dist/ipf
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/dist/ipf/netinet
CPPFLAGS+=	-DSTATETOP -D__UIO_EXPOSE

IPFOBJDIR!=	cd $(.CURDIR)/../libipf && ${PRINTOBJDIR}
DPADD+=		${IPFOBJDIR}/libipf.a ${LIBKVM}
LDADD+=		-L${IPFOBJDIR} -lipf -lkvm

CLEANFILES+=	y.tab.c y.tab.h

.PATH:	${NETBSDSRCDIR}/dist/ipf	\
	${NETBSDSRCDIR}/dist/ipf/lib	\
	${NETBSDSRCDIR}/dist/ipf/tools	\
	${NETBSDSRCDIR}/dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
