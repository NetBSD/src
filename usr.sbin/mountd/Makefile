#	@(#)Makefile	5.4 (Berkeley) 6/29/90
#
#	$Header: /cvsroot/src/usr.sbin/mountd/Makefile,v 1.5 1993/06/15 23:13:33 mycroft Exp $

PROG=	mountd
CFLAGS+=-DNFS -DNFSSERVER
MAN5=	exports.0
MAN8=	mountd.0
DPADD=	${LIBRPC} ${LIBUTIL}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
