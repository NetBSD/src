#	$NetBSD: Makefile.inc,v 1.1.4.1 2014/08/19 23:46:47 tls Exp $

WARNS?=	1	# XXX -Wcast-qual -Wshadow
CWARNFLAGS.clang+=	-Wno-format -Wno-tautological-compare \
			-Wno-self-assign -Wno-array-bounds \
			-Wno-error=unused-const-variable

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server

UDIST=${NETBSDSRCDIR}/external/bsd/ipf/dist
SDIST=${NETBSDSRCDIR}/sys/external/bsd/ipf

CPPFLAGS+=	-I${UDIST}
CPPFLAGS+=	-I${UDIST}/tools
CPPFLAGS+=	-I${SDIST}
CPPFLAGS+=	-I${SDIST}/netinet
CPPFLAGS+=	-DSTATETOP -D__UIO_EXPOSE
CPPFLAGS+=	-DINET

.if (${USE_INET6} == "no")
CPPFLAGS+=	-DNOINET6
.else
CPPFLAGS+=	-DINET6
.endif

IPFOBJDIR!=	cd ${UDIST}/../lib && ${PRINTOBJDIR}
DPADD+=		${IPFOBJDIR}/libipf.a ${LIBKVM}
LDADD+=		-L${IPFOBJDIR} -lipf -lkvm

CLEANFILES+=	y.tab.c y.tab.h

.PATH:	${UDIST}	\
	${UDIST}/lib	\
	${UDIST}/tools	\
	${UDIST}/man
