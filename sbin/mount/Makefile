#	@(#)Makefile	5.5 (Berkeley) 5/11/90
#
#	$Id: Makefile,v 1.2 1993/03/22 08:04:00 cgd Exp $

PROG=	mount
CFLAGS+=-DNFS
MAN8=	mount.0
DPADD=	${LIBRPC}
LDADD=	-lrpc
MLINKS=	mount.8 umount.8

.include <bsd.prog.mk>
