#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	from: BSDI Id: Makefile,v 1.3 1993/06/14 20:06:22 donn Exp 
#	$Id: Makefile,v 1.12 1993/10/07 02:18:21 cgd Exp $

PROG=	init
MAN8=	init.0
# XXX fix this when we have KERN_SECURELVL
CFLAGS+=-DSECURE -DNOSYSCTL
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=500

.include <bsd.prog.mk>
