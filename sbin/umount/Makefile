#	@(#)Makefile	5.4 (Berkeley) 5/11/90
#
#	$Id: Makefile,v 1.2 1993/03/22 08:04:00 cgd Exp $

PROG=	umount
CFLAGS+=-DNFS
NOMAN=	noman
DPADD=	${LIBRPC}
LDADD=	-lrpc

.include <bsd.prog.mk>
