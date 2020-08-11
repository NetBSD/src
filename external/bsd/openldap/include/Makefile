#	$NetBSD: Makefile,v 1.2 2020/08/11 13:15:43 christos Exp $

.include "../openldap.mk"

.PATH:	${LDAP_DISTDIR}/include

INCS=	lber.h lber_types.h \
	ldap.h ldap_cdefs.h \
	ldap_features.h ldap_schema.h ldap_utf8.h openldap.h

INCSDIR=/usr/include

.include <bsd.prog.mk>
