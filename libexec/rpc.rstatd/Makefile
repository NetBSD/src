#
# a makefile for /usr/sbin/rpc.rstatd
#
#	$Header: /cvsroot/src/libexec/rpc.rstatd/Makefile,v 1.1 1993/06/03 19:26:15 brezak Exp $
#

PROG =	rpc.rstatd
SRCS =	rstatd.c rstat_proc.c
MAN8=	rpc.rstatd.0

DPADD=	${LIBRPCSVC} ${LIBRPC} ${LIBUTIL}
LDADD=	${LDARGS} -lrpcsvc -lrpc -lutil

.include <bsd.prog.mk>
