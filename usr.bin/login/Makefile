#	from: @(#)Makefile	5.6 (Berkeley) 6/24/90
#	$Id: Makefile,v 1.6 1993/10/07 02:19:25 cgd Exp $

CFLAGS+=#-DKERBEROS
PROG=	login
SRCS=	klogin.c login.c
DPADD=	${LIBUTIL} ${LIBCRYPT} #${LIBKRB} ${LIBDES}
LDADD=	-lutil -lcrypt #-lkrb -ldes

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
