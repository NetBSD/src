#	$Id: Makefile,v 1.5 1994/01/28 00:30:52 cgd Exp $

PROG =	rpc.rstatd
SRCS =	rstatd.c rstat_proc.c
MAN8 =	rpc.rstatd.0

DPADD=	${LIBRPCSVC} ${LIBKVM}
LDADD=	-lrpcsvc -lc -lkvm	# XXX (for getloadavg())

.include <bsd.prog.mk>
