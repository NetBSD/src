#	$NetBSD: Makefile,v 1.39 2003/07/23 08:01:35 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=	login
SRCS=	login.c
DPADD+=	${LIBUTIL} ${LIBCRYPT}
LDADD+=	-lutil -lcrypt
BINOWN=	root
BINMODE=4555
CPPFLAGS+=-DLOGIN_CAP -DSUPPORT_UTMP -DSUPPORT_UTMPX

.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+=	-lskey
.endif

.if (${USE_KERBEROS} != "no")
SRCS+= k5login.c
CPPFLAGS+=-DKERBEROS5 -I${DESTDIR}/usr/include/krb5
DPADD+=	${LIBKRB5} ${LIBASN1}
LDADD+= -lkrb5 -lasn1
.endif

.if (${USE_KERBEROS4} != "no")
SRCS+= klogin.c
CPPFLAGS+=-DKERBEROS -I${DESTDIR}/usr/include/kerberosIV
DPADD+=	${LIBKRB}
LDADD+= -lkrb
.endif

.if (${USE_KERBEROS4} != "no" || ${USE_KERBEROS} != "no")
DPADD+=	${LIBCRYPTO} ${LIBROKEN} ${LIBCOM_ERR}
LDADD+=	-lcrypto -lroken -lcom_err
.endif

.include <bsd.prog.mk>
