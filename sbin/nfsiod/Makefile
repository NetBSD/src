#	@(#)Makefile	5.3 (Berkeley) 6/29/90
#
#	$Header: /cvsroot/src/sbin/nfsiod/Attic/Makefile,v 1.4 1993/06/15 23:13:39 mycroft Exp $

PROG=	nfsiod
CFLAGS+=-DNFS
MAN8=	nfsiod.0
DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
