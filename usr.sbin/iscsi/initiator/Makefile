# $NetBSD: Makefile,v 1.2 2009/06/19 16:13:49 agc Exp $

.include <bsd.own.mk>

PROG=iscsi-initiator
SRCS=iscsi-initiator.c virtdir.c initiator.c
LDADD+= -lrefuse -liscsi
CPPFLAGS+= -g -I${NETBSDSRCDIR}/dist/iscsi/include
MAN=iscsi-initiator.8
WARNS=4

.PATH: ${NETBSDSRCDIR}/dist/iscsi/src

.include <bsd.prog.mk>
