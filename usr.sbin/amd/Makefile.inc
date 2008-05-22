#	$NetBSD: Makefile.inc,v 1.22 2008/05/22 14:48:15 lukem Exp $

WARNS?=	1

.include <bsd.own.mk>

USE_FORT?=yes

IDIST=	${NETBSDSRCDIR}/dist/am-utils

LIBAMUOBJDIR!=cd ${.CURDIR}/../libamu && ${PRINTOBJDIR}

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}/libamu \
	-I${IDIST}/include -I${LIBAMUOBJDIR}

.if (${USE_HESIOD} != "no")
CPPFLAGS+=-DHAVE_MAP_HESIOD=1
.endif

.if (${USE_LDAP} != "no")
CPPFLAGS+=-DHAVE_MAP_LDAP=1 -DHAVE_LBER_H=1 -DHAVE_LDAP_H -DHAVE_LDAP_OPEN=1
CPPFLAGS+=-DLDAP_DEPRECATED=1	# OpenLDAP 2.2 deprecated ldap_unbind()
.endif

.if (${USE_YP} != "no")
CPPFLAGS+=-DHAVE_MAP_NIS=1
.endif

.if !defined(LIB) || empty(LIB)
LDADD+=	-L${LIBAMUOBJDIR} -lamu
.if ${MKPICLIB} == "no"
DPADD+=	${LIBAMUOBJDIR}/libamu.a
.else
DPADD+=	${LIBAMUOBJDIR}/libamu_pic.a
.endif
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
