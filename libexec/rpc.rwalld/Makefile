#
# a makefile for /usr/sbin/rpc.rwalld
#
#	$Header: /cvsroot/src/libexec/rpc.rwalld/Makefile,v 1.2 1993/06/04 14:55:58 brezak Exp $
#

PROG =	rpc.rwalld
SRCS =	rwalld.c
NOMAN=	noman

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

.include <bsd.prog.mk>
