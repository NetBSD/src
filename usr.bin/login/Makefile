#	from: @(#)Makefile	5.6 (Berkeley) 6/24/90
#	$Id: Makefile,v 1.8 1994/05/24 06:50:57 deraadt Exp $

PROG=	login
SRCS=	klogin.c login.c
CFLAGS+=-DSKEY
DPADD=	${LIBUTIL} ${LIBCRYPT} ${LIBSKEY}
LDADD=	-lutil -lcrypt -lskey

.if defined(KERBEROS)
CFLAGS+= -DKERBEROS
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
