#
# a makefile for /usr/sbin/rpc.rwalld
#
#	$Header: /cvsroot/src/libexec/rpc.rwalld/Makefile,v 1.1 1993/06/03 19:26:30 brezak Exp $
#

PROG =	rpc.rwalld
SRCS =	rwalld.c
NOMAN=	noman

DPADD=	${LIBRPCSVC} ${LIBRPC} ${LIBUTIL}
LDADD=	${LDARGS} -lrpcsvc -lrpc -lutil

.include <bsd.prog.mk>
