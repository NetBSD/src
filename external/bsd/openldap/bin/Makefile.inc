#	$NetBSD: Makefile.inc,v 1.4 2013/09/11 23:04:10 joerg Exp $

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

LDADD+= -lldap
DPADD+= ${LIBLDAP}
LDADD+=	-llber
DPADD+=	${LIBLBER}

.if (${MKPIC} == "no" || (defined(LDSTATIC) && ${LDSTATIC} != ""))
.if ${MKCRYPTO} != "no" 
DPADD+=	${LIBSSL} ${LIBCRYPTO}
LDADD+=	-lssl -lcrypto
.endif
.endif
