#
# a makefile for /usr/sbin/rpc.rwalld
#
#	/b/source/CVS/src/libexec/rpc.rwalld/Makefile,v 1.2 1993/06/04 14:55:58 brezak Exp
#

PROG =	rpc.rwalld
SRCS =	rwalld.c
MAN8 =	rpc.rwalld.0

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

.include <bsd.prog.mk>
