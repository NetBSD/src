#	@(#)Makefile	5.5 (Berkeley) 5/11/90
#
#	$Header: /cvsroot/src/sbin/mount/Makefile,v 1.4 1993/04/12 18:22:27 cgd Exp $

PROG=	mount
CFLAGS+=-DNFS -DNFSCLIENT
MAN8=	mount.0
DPADD=	${LIBRPC}
LDADD=	-lrpc
MLINKS=	mount.8 umount.8

.include <bsd.prog.mk>
