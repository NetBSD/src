#	$NetBSD: Makefile.inc,v 1.1 2021/12/28 17:05:44 christos Exp $

.include "../openldap.mk"

BINDIR=	/usr/sbin

LDADD+=	-L${LDAPOBJDIR.rewrite} -lrewrite
DPADD+=	${LDAPLIB.rewrite}
LDADD+=	-L${LDAPOBJDIR.lunicode} -llunicode
DPADD+=	${LDAPLIB.lunicode}
LDADD+=	-L${LDAPOBJDIR.lutil} -llutil
DPADD+=	${LDAPLIB.lutil}

LDADD+= -lldap_r ${LIBLDAP_LDADD}
DPADD+= ${LIBLDAP_DPADD}
