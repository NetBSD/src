#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	from: BSDI Id: Makefile,v 1.3 1993/06/14 20:06:22 donn Exp 
#	$Id: Makefile,v 1.8 1993/06/18 21:16:25 cgd Exp $

PROG=	init
MAN8=	init.0
# XXX fix this when we have KERN_SECURELVL
CFLAGS+=-DSECURE -DNOSYSCTL -DDEBUGSHELL -DCONSOLENAME=\"vga\"
DPADD=	${LIBUTIL}
LDADD=	-lutil
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif
BINMODE=500

.include <bsd.prog.mk>
