#	@(#)Makefile	5.3 (Berkeley) 6/29/90
#
#	$Header: /cvsroot/src/sbin/nfsd/Attic/Makefile,v 1.3 1993/03/23 00:29:28 cgd Exp $

PROG=	nfsd
CFLAGS+=-DNFS
MAN8=	nfsd.0
DPADD=	${LIBRPC}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
