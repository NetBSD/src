#	$NetBSD: Makefile,v 1.17 1997/10/12 13:10:16 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

WARNS?= 1
PROG=	login
SRCS=	login.c
DPADD+=	${LIBUTIL} ${LIBCRYPT}
LDADD+=	-lutil -lcrypt
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

.include <bsd.prog.mk>

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
