#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.4 1994/06/28 07:47:23 deraadt Exp $

PROG=	quota
BINOWN=	root
BINMODE=4555

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
