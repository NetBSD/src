#	@(#)Makefile	5.3 (Berkeley) 6/29/90
#
#	$Id: Makefile,v 1.2 1993/03/22 08:04:00 cgd Exp $

PROG=	nfsd
CFLAGS+=-DNFS
MAN8=	nfsd.0
DPADD=	${LIBRPC}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
