#	from: @(#)Makefile	5.3 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.6 1993/10/08 02:30:26 cgd Exp $

PROG=	nfsd
CFLAGS+=-DNFS
MAN8=	nfsd.0
DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
