#	$NetBSD: Makefile,v 1.13 1997/08/19 17:26:13 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	login
SRCS=	login.c
DPADD=	${LIBUTIL} ${LIBCRYPT} ${LIBSKEY}
LDADD=	-lutil -lcrypt -lskey
CFLAGS+= -DSKEY
LDSTATIC= -static
WARNS?= 1

.if defined(KERBEROS5)
SRCS+=	k5login.c
DPADD+=	${LIBKRB5} ${LIBCRYPTO}
LDADD+=	-lkrb5 -lcrypto -lcom_err
CFLAGS+= -DKERBEROS5
.elif defined(KERBEROS)
SRCS+=	klogin.c
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
CFLAGS+= -DKERBEROS
.endif

BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

.include <bsd.prog.mk>
