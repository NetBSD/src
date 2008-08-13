#	$NetBSD: Makefile.inc,v 1.2 2008/08/13 04:43:26 lukem Exp $

.include "../openldap.mk"

BINDIR=	/usr/bin

.PATH:	${LDAP_DISTDIR}/clients/tools
.PATH:	${LDAP_SRCDIR}/man

SRCS=	${PROG}.c common.c


SRCS+=	version.c
CLEANFILES+= version.c

version.c: ../openldap.mk
	${_MKTARGET_CREATE}
	${LDAP_MKVERSION} -s ${PROG} > ${.TARGET}


LDADD+=	-L${LDAPOBJDIR.lutil} -llutil
DPADD+=	${LDAPLIB.lutil}

LDADD+= -lldap
DPADD+= ${LIBLDAP}

.if (${MKPIC} == "no" || (defined(LDSTATIC) && ${LDSTATIC} != ""))
LDADD+=	-llber
DPADD+=	${LIBLBER}
.if ${MKCRYPTO} != "no" 
DPADD+=	${LIBSSL} ${LIBCRYPTO}
LDADD+=	-lssl -lcrypto
.endif
.endif
