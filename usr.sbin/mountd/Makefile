#	from: @(#)Makefile	5.4 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.6 1993/08/01 05:37:30 mycroft Exp $

PROG=	mountd
CFLAGS+=-DNFS -DNFSSERVER
MAN5=	exports.0
MAN8=	mountd.0
DPADD=	${LIBRPC} ${LIBUTIL}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
