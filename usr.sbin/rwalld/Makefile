#
# a makefile for /usr/sbin/rwalld
#
#	$Header: /cvsroot/src/usr.sbin/rwalld/Attic/Makefile,v 1.1 1993/04/04 19:01:05 cgd Exp $
#

PROG = rwalld
SRCS = rwalld.c rwall_svc.c
CLEANFILES += rwall_svc.c
NOMAN=	noman

RPCCOM = rpcgen
RPCDLOC= /usr/include
RPCDESC= rpcsvc/rwall.x

DPADD=	${LIBRPC}
LDADD=	-lrpc

rwall_svc.c: ${RPCDLOC}/${RPCDESC}
	@echo generating $@...
	@PWD=`pwd` ; cd ${RPCDLOC} ; ${RPCCOM} -m ${RPCDESC} -o $$PWD/$@

.include <bsd.prog.mk>
