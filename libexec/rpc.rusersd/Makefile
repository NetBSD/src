#
# a makefile for /usr/sbin/rpc.rusersd
#
#	/b/source/CVS/src/libexec/rpc.rusersd/Makefile,v 1.2 1993/06/04 14:55:55 brezak Exp
#

PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c
MAN8 =	rpc.rusersd.0

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

.include <bsd.prog.mk>

