#	$NetBSD: Makefile,v 1.19 1997/10/19 03:44:25 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	login
SRCS=	login.c
DPADD+=	${LIBUTIL} ${LIBCRYPT}
LDADD+=	-lutil -lcrypt
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

.include <bsd.own.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+=	-lskey
.endif

.ifdef KERBEROS5
SRCS+=	k5login.c
CPPFLAGS+=-DKERBEROS5
DPADD+=	${LIBKRB5} ${LIBCRYPTO}
LDADD+=	-lkrb5 -lcrypto -lcom_err
.elifdef KERBEROS
SRCS+=	klogin.c
CPPFLAGS+=-DKERBEROS
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif

.include <bsd.prog.mk>
