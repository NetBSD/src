#	@(#)Makefile	5.4 (Berkeley) 5/11/90
#
#	$Header: /cvsroot/src/sbin/umount/Makefile,v 1.3 1993/03/23 00:31:24 cgd Exp $

PROG=	umount
CFLAGS+=-DNFS
NOMAN=	noman
DPADD=	${LIBRPC}
LDADD=	-lrpc

.include <bsd.prog.mk>
