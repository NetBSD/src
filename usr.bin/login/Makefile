#	$NetBSD: Makefile,v 1.12 1997/08/16 13:50:43 lukem Exp $
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
LDADD+=	-lkrb5 -lcrypto
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
