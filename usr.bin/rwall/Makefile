#
# a makefile for /usr/bin/rwall
#
# $Header: /cvsroot/src/usr.bin/rwall/Makefile,v 1.1 1993/04/04 20:15:30 cgd Exp $
#

PROG = rwall
SRCS = rwall.c rwall_clnt.c
CLEANFILES += rwall_clnt.c
NOMAN=	noman

RPCCOM = rpcgen
RPCDLOC= /usr/include
RPCDESC= rpcsvc/rwall.x

DPADD=	${LIBRPC}
LDADD=	-lrpc

rwall_clnt.c: ${RPCDLOC}/${RPCDESC}
	@echo generating $@...
	@PWD=`pwd` ; cd ${RPCDLOC} ; ${RPCCOM} -l ${RPCDESC} -o $$PWD/$@

.include <bsd.prog.mk>
