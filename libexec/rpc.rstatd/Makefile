#
# a makefile for /usr/sbin/rpc.rstatd
#
#	$Header: /cvsroot/src/libexec/rpc.rstatd/Makefile,v 1.2 1993/06/04 14:55:52 brezak Exp $
#

PROG =	rpc.rstatd
SRCS =	rstatd.c rstat_proc.c
MAN8=	rpc.rstatd.0

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

.include <bsd.prog.mk>
