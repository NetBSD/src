#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93
#	$Id: Makefile,v 1.9 1994/07/25 21:11:08 brezak Exp $

PROG=	login
SRCS=	klogin.c k5login.c login.c
DPADD=	${LIBUTIL} ${LIBCRYPT} ${LIBSKEY}
LDADD=	-lutil -lcrypt -lskey
CFLAGS+= -DSKEY

.if defined(KERBEROS5)
CFLAGS+= -DKERBEROS5
DPADD+=	${LIBKRB5} ${LIBCRYPTO}
LDADD+=	-lkrb5 -lcrypto
.elif defined(KERBEROS)
CFLAGS+= -DKERBEROS
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
