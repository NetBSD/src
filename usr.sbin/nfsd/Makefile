#	from: @(#)Makefile	5.3 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.5 1993/08/01 05:27:26 mycroft Exp $

PROG=	nfsd
CFLAGS+=-DNFS
MAN8=	nfsd.0
DPADD=	${LIBRPC} ${LIBUTIL}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
