#
# a makefile for /usr/sbin/rpc.rusersd
#
#	$Header: /cvsroot/src/libexec/rpc.rusersd/Makefile,v 1.1 1993/06/03 19:26:21 brezak Exp $
#

PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c
NOMAN=	noman

DPADD=	${LIBRPCSVC} ${LIBRPC} ${LIBUTIL}
LDADD=	${LDARGS} -lrpcsvc -lrpc -lutil

.include <bsd.prog.mk>
