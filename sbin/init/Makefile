#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	from: BSDI Id: Makefile,v 1.3 1993/06/14 20:06:22 donn Exp 
#	$Id: Makefile,v 1.9 1993/07/08 01:47:31 cgd Exp $

PROG=	init
MAN8=	init.0
# XXX fix this when we have KERN_SECURELVL
CFLAGS+=-DSECURE -DNOSYSCTL -DDEBUGSHELL
DPADD=	${LIBUTIL}
LDADD=	-lutil
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif
BINMODE=500

.include <bsd.prog.mk>
