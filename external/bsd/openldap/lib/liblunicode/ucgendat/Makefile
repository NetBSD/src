# $NetBSD: Makefile,v 1.1 2021/12/28 17:05:23 christos Exp $

.include <bsd.hostinit.mk>
.include <${.CURDIR}/../../../openldap.mk>

.PATH:	${LDAP_DISTDIR}/libraries/liblunicode/ucdata

PROG= ucgendat

SRCS+=	ucgendat.c

CPPFLAGS+=				\
	-I${LDAP_SRCDIR}/include	\
	-I${LDAP_DISTDIR}/include	\
	'-Dlutil_progname(a,b,c)=__UNCONST(getprogname())'

HOST_CPPFLAGS+= ${CPPFLAGS}

.ifndef HOSTPROG
HOSTPROG=	${PROG}
.include <bsd.hostprog.mk>
.endif
