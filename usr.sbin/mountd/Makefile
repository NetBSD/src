#	@(#)Makefile	5.4 (Berkeley) 6/29/90
#
#	$Id: Makefile,v 1.2 1993/03/22 08:04:00 cgd Exp $

PROG=	mountd
CFLAGS+=-DNFS
MAN5=	exports.0
MAN8=	mountd.0
DPADD=	${LIBRPC}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
