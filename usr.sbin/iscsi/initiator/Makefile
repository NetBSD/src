# $NetBSD: Makefile,v 1.1 2009/06/19 07:17:09 agc Exp $

.include <bsd.own.mk>

PROG=iscsi-initiator
SRCS=iscsi-initiator.c virtdir.c initiator.c
LDADD+= -lrefuse -liscsi
CPPFLAGS+= -g -I${NETBSDSRCDIR}/dist/iscsi/include
MAN=iscsi-initiator.8
WARNS=3

.PATH: ${NETBSDSRCDIR}/dist/iscsi/src

.include <bsd.prog.mk>
