#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	from: BSDI Id: Makefile,v 1.3 1993/06/14 20:06:22 donn Exp 
#	$Id: Makefile,v 1.1 1994/07/30 19:30:42 chopps Exp $

PROG=	init
SRCS=	init.c
NOMAN=
CFLAGS+=-DNOSYSCTL -DLETS_GET_SMALL
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=500

BINDIR=	/sbin

.PATH:	${.CURDIR}/../../../../../sbin/init

.include <bsd.prog.mk>
