#	@(#)Makefile	5.4 (Berkeley) 6/29/90
#
#	$Header: /cvsroot/src/sbin/mountd/Attic/Makefile,v 1.4 1993/04/12 19:31:40 cgd Exp $

PROG=	mountd
CFLAGS+=-DNFS -DNFSSERVER
MAN5=	exports.0
MAN8=	mountd.0
DPADD=	${LIBRPC}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
