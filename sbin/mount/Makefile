#	from: @(#)Makefile	5.5 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.5 1993/08/01 05:37:33 mycroft Exp $

PROG=	mount
CFLAGS+=-DNFS -DNFSCLIENT
MAN8=	mount.0
DPADD=	${LIBRPC}
LDADD=	-lrpc
MLINKS=	mount.8 umount.8

.include <bsd.prog.mk>
