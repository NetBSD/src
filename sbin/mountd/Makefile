#	@(#)Makefile	5.4 (Berkeley) 6/29/90
#
#	$Header: /cvsroot/src/sbin/mountd/Attic/Makefile,v 1.3 1993/03/23 00:29:03 cgd Exp $

PROG=	mountd
CFLAGS+=-DNFS
MAN5=	exports.0
MAN8=	mountd.0
DPADD=	${LIBRPC}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
