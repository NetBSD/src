#	$NetBSD: openldap.mk,v 1.7 2017/05/21 15:28:39 riastradh Exp $

.include <bsd.own.mk>

LDAP_VERSION=	2.4.23

LDAP_SRCDIR=	${NETBSDSRCDIR}/external/bsd/openldap
LDAP_DISTDIR=	${NETBSDSRCDIR}/external/bsd/openldap/dist

LDAP_PREFIX=	/usr

LDAP_DATADIR=	${LDAP_PREFIX}/share/openldap
LDAP_ETCDIR=	/etc
LDAP_RUNDIR=	/var/openldap

CPPFLAGS+=	-I${LDAP_SRCDIR}/include
CPPFLAGS+=	-I${LDAP_DISTDIR}/include

.for _LIB in lutil ldap		# XXX lber ldap_r lunicode rewrite
.if !defined(LDAPOBJDIR.${_LIB})
LDAPOBJDIR.${_LIB}!=	cd ${LDAP_SRCDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
.MAKEOVERRIDES+=	LDAPOBJDIR.${_LIB}
.endif
LDAPLIB.${_LIB}=	${LDAPOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

LDAP_MKVERSION=	${HOST_SH} ${LDAP_DISTDIR}/build/mkversion -v "${LDAP_VERSION}"

CPPFLAGS+=	-DHAVE_TLS
