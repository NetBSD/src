#	from: @(#)Makefile	5.3 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.5 1993/08/01 05:27:16 mycroft Exp $

PROG=	nfsiod
CFLAGS+=-DNFS
MAN8=	nfsiod.0
DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
