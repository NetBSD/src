#	@(#)Makefile	5.3 (Berkeley) 6/29/90
#
#	$Header: /cvsroot/src/usr.sbin/nfsd/Makefile,v 1.4 1993/06/15 23:13:37 mycroft Exp $

PROG=	nfsd
CFLAGS+=-DNFS
MAN8=	nfsd.0
DPADD=	${LIBRPC} ${LIBUTIL}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
