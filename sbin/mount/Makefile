#	@(#)Makefile	5.5 (Berkeley) 5/11/90
#
#	$Header: /cvsroot/src/sbin/mount/Makefile,v 1.3 1993/03/23 00:28:48 cgd Exp $

PROG=	mount
CFLAGS+=-DNFS
MAN8=	mount.0
DPADD=	${LIBRPC}
LDADD=	-lrpc
MLINKS=	mount.8 umount.8

.include <bsd.prog.mk>
