#	$NetBSD: Makefile.inc,v 1.2.26.1 2013/02/25 00:24:52 tls Exp $

.ifndef AMU_MAKEFILE_INC
AMU_MAKEFILE_INC=
WARNS?=	1

.include <bsd.own.mk>

BINDIR?= /usr/sbin

USE_FORT?=yes

AMUTILS=	${NETBSDSRCDIR}/external/bsd/am-utils
IDIST=		${AMUTILS}/dist

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../../include -I${IDIST}/libamu \
    -I${IDIST}/include

LIBAMUOBJDIR!=	cd ${AMUTILS}/lib/libamu && ${PRINTOBJDIR}
CPPFLAGS+=	-I${LIBAMUOBJDIR}

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
PROGDPLIBS+=	amu	${AMUTILS}/lib/libamu
.endif
.endif
