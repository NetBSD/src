#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.3 1994/01/28 00:58:41 cgd Exp $

PROG=	rwhod
MAN8=	rwhod.0
DPADD=	${LIBKVM}
LDADD=	-lc -lkvm 	# XXX (for getloadavg())

.include <bsd.prog.mk>
