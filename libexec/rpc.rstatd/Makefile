#
# a makefile for /usr/sbin/rpc.rstatd
#
#	$Header: /cvsroot/src/libexec/rpc.rstatd/Makefile,v 1.3 1993/06/08 01:19:09 brezak Exp $
#

PROG =	rpc.rstatd
SRCS =	rstatd.c rstat_proc.c
MAN8 =	rpc.rstatd.0

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

.include <bsd.prog.mk>
