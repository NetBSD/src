#	from: @(#)Makefile	5.6 (Berkeley) 6/24/90
#	$Id: Makefile,v 1.7 1994/03/30 02:49:15 cgd Exp $

PROG=	login
SRCS=	klogin.c login.c
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt

.if defined(KERBEROS)
CFLAGS+= -DKERBEROS
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
