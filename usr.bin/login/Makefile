#	$NetBSD: Makefile,v 1.31 2000/07/23 22:23:14 mycroft Exp $
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
LDADD+= -lkrb5 -lasn1

SRCS+= klogin.c
CPPFLAGS+=-DKERBEROS -I${DESTDIR}/usr/include/kerberosIV
LDADD+= -lkrb

LDADD+=	-lcrypto -lroken -lcom_err
.endif

.include <bsd.prog.mk>
