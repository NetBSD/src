#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.4 1993/07/31 15:34:25 mycroft Exp $

PROG=	umount
CFLAGS+=-DNFS
NOMAN=	noman
DPADD=	${LIBRPC}
LDADD=	-lrpc

.include <bsd.prog.mk>
