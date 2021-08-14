#	$NetBSD: Makefile.inc,v 1.7 2021/08/14 16:14:49 christos Exp $

.include "../openldap.mk"

BINDIR=	/usr/bin

.PATH:	${LDAP_DISTDIR}/clients/tools
.PATH:	${LDAP_SRCDIR}/man

SRCS=	${PROG}.c
.if !defined(NO_COMMON_C)
SRCS+=	common.c
.endif


SRCS+=	version.c
CLEANFILES+= version.c

version.c: ../openldap.mk
	${_MKTARGET_CREATE}
	${LDAP_MKVERSION} -s ${PROG} > ${.TARGET}


LDADD+=	-L${LDAPOBJDIR.lutil} -llutil
DPADD+=	${LDAPLIB.lutil}

LDADD+= ${LIBLDAP_LDADD}
DPADD+= ${LIBLDAP_DPADD}
