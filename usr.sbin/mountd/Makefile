#	from: @(#)Makefile	5.4 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.7 1993/10/08 02:28:39 cgd Exp $

PROG=	mountd
CFLAGS+=-DNFS -DNFSSERVER
MAN5=	exports.0
MAN8=	mountd.0
DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
