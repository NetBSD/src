#	$NetBSD: Makefile,v 1.15 1997/10/12 12:39:17 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

WARNS?= 1
PROG=	login
SRCS=	login.c
DPADD+=	${LIBUTIL} ${LIBCRYPT}
LDADD+=	-lutil -lcrypt
LDSTATIC= -static

.include <bsd.own.mk>

.if defined(SKEY)
CPPFLAGS+= -DSKEY
DPADD+= ${LIBSKEY}
LDADD+=	-lskey
.endif

.if defined(KERBEROS5)
SRCS+=	k5login.c
DPADD+=	${LIBKRB5} ${LIBCRYPTO}
LDADD+=	-lkrb5 -lcrypto -lcom_err
CPPFLAGS+= -DKERBEROS5
.elif defined(KERBEROS)
SRCS+=	klogin.c
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
CPPFLAGS+= -DKERBEROS
.endif

BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

.include <bsd.prog.mk>
