#	$NetBSD: Makefile,v 1.29.2.2 2000/06/24 06:59:36 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=	login
SRCS=	login.c
DPADD+=	${LIBUTIL} ${LIBCRYPT}
LDADD+=	-lutil -lcrypt
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg
CPPFLAGS+=-DLOGIN_CAP

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+=	-lskey
.endif

.if (${MKKERBEROS} != "no")
SRCS+= k5login.c
CPPFLAGS+=-DKERBEROS5 -I${DESTDIR}/usr/include/krb5
LDADD+= -lkrb5 -lcrypto -lasn1 -lcom_err -lroken

SRCS+= klogin.c
CPPFLAGS+=-DKERBEROS -I${DESTDIR}/usr/include/kerberosIV
LDADD+= -lkrb
.endif

.include <bsd.prog.mk>
